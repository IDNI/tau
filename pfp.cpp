#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include <iostream>
#include <random>
#include <sstream>
#include <climits>
using namespace std;

typedef int32_t int_t;
typedef array<int_t, 3> node; // [bdd] node is a triple: varid, 1-node-id, 0-node-id
typedef const wchar_t* wstr;
template<typename K> using matrix = vector<vector<K>>; // used as a set of terms (e.g. rule)
typedef vector<bool> bits;
typedef vector<bits> vbits;
#define er(x)	perror(x), exit(0)
#define oparen_expected "'(' expected\n"
#define comma_expected "',' or ')' expected\n"
#define dot_after_q "expected '.' after query.\n"
#define if_expected "'if' or '.' expected\n"
#define sep_expected "Term or ':-' or '.' expected\n"
#define unmatched_quotes "Unmatched \"\n"
#define err_inrel "Unable to read the input relation symbol.\n"
#define err_src "Unable to read src file.\n"
#define err_dst "Unable to read dst file.\n"
////////////////////////////////////////////////////////////////////////////////////////////////////
struct rule { // a [P-DATALOG] rule in bdd form with additional information
	bool neg;
	int_t h; // bdd root
	size_t w; // nbodies, will determine the virtual power
	set<int_t> x; // existentials
	map<int_t, int_t> hvars; // how to permute body vars to head vars
};
////////////////////////////////////////////////////////////////////////////////////////////////////
class bdds_base {
	vector<node> V; // all nodes
	map<node, int_t> M; // node to its index
	int_t root; // used for implicit power
	size_t dim = 1; // used for implicit power
protected:
	int_t add(const node& n);
	int_t add_nocheck(const node& n) { return V.emplace_back(n), M[n]=V.size()-1; }
	bdds_base() { add_nocheck({0, 0, 0}), add_nocheck({0, 1, 1}); }
public:
	static const int_t F = 0, T = 1;
	node getnode(size_t n) const; // node from id. equivalent to V[n] unless virtual pow is used
	void setpow(int_t _root, size_t _dim) { root = _root, dim = _dim; }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
// the following to be used with bdds::apply()
struct op_or_t { int_t operator()(int_t x, int_t y) const { return (x||y)?1:0; } } op_or; 
struct op_and_t { int_t operator()(int_t x, int_t y) const { return (x&&y)?1:0; } } op_and; 
struct op_and_not_t { int_t operator()(int_t x, int_t y) const { return (x&&!y)?1:0; } } op_and_not;
////////////////////////////////////////////////////////////////////////////////////////////////////
vbits& operator*=(vbits& x, const pair<const vbits&, size_t>& y); // to be used with allsat()

class bdds : public bdds_base { // holding functions only, therefore tbd: dont use it as an object
	int_t from_bit(int_t x, bool v) { return add(v ? node{x, T, F} : node{x, F, T}); }
	size_t count(int_t x) const;
	vbits& sat(int_t x, vbits& r) const;
public:
	template<typename op_t> static // binary application
	int_t apply(const bdds& bx, int_t x, const bdds& by, int_t y, bdds& r, const op_t& op);
	template<typename op_t> static int_t apply(const bdds& b, int_t x, bdds& r, const op_t& op);
	template<typename op_t> static int_t apply(bdds& b, int_t x,bdds& r, const op_t& op);//unary
	static int_t permute(bdds& b, int_t x, bdds& r, const map<int_t, int_t>&);
	// helper constructors
	int_t from_eq(int_t x, int_t y); // a bdd saying "x=y"
	template<typename K> rule from_rule(matrix<K> v, const size_t bits, const size_t ar);
	// helper apply() variations
	int_t bdd_or(int_t x, int_t y)	{ return apply(*this, x, *this, y, *this, op_or); } 
	int_t bdd_and(int_t x, int_t y)	{ return apply(*this, x, *this, y, *this, op_and); } 
	int_t bdd_and_not(int_t x, int_t y){ return apply(*this, x, *this, y, *this, op_and_not); }
	// count/return satisfying assignments
	size_t satcount(int_t x) const	{ return x < 2 ? x : (count(x) << (getnode(x)[0] - 1)); }
	vbits allsat(int_t x) const;
	// print a bdd, using ?: syntax
	void out(wostream& os, const node& n) const;
	void out(wostream& os, size_t n) const	{ out(os, getnode(n)); }
};

struct op_exists { // existential quantification, to be used with apply()
	const set<int_t>& s;
	op_exists(const set<int_t>& s) : s(s) { }
	node operator()(bdds& b, const node& n) const {
		return s.find(n[0]) == s.end() ? n : b.getnode(b.bdd_or(n[1], n[2]));
	}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename K> class dict_t { // handles representation of strings as unique integers
	struct dictcmp {
		bool operator()(const pair<wstr, size_t>& x, const pair<wstr, size_t>& y) const;
	};
	map<pair<wstr, size_t>, K, dictcmp> syms_dict, vars_dict;
	vector<wstr> syms;
	vector<size_t> lens;
public:
	const K pad = 1;
	dict_t() { syms.push_back(0), lens.push_back(0), syms_dict[{0, 0}] = pad; }
	K operator()(wstr s, size_t len);
	pair<wstr, size_t> operator()(K t) { return { syms[t-1], lens[t-1] }; }
	size_t bits() const { return (sizeof(K)<<3) - __builtin_clz(syms.size()); }
};
////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename K> class lp { // [pfp] logic program
	dict_t<K> dict; // hold its own dict so we can determine the universe size
	bdds prog, dbs; // separate bdds for prog and db cause db is a virtual power
	vector<rule> rules; // prog's rules

	K str_read(wstr *s); // parser's helper, reads a string and returns its dict id
	vector<K> term_read(wstr *s); // read raw term (no bdd)
	matrix<K> rule_read(wstr *s); // read raw rule (no bdd)
public:
	int_t db; // db's bdd root
	void prog_read(wstr s);
	void step(); // single pfp step
	void printdb(wostream&) const;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
int_t bdds_base::add(const node& n) { // create new bdd node, standard implementation
	if (n[1] == n[2]) return n[1];
	auto it = M.find(n);
	return it == M.end() ? add_nocheck(n) : it->second;
}

node bdds_base::getnode(size_t n) const { // returns a bdd node considering virtual powers
	if (dim == 1) return V[n];
	const size_t m = n % V.size(), ms = (n / V.size() + 1) * V.size();
	node r = V[m];
	return r[0] ? r[1] += ms, r[2] += ms, r : r[1] ? ms == V.size()*dim ? V[T] : V[root] : V[F];
}

size_t bdds::count(int_t x) const {
	node n = getnode(x), k;
	if (!n[0]) return n[1];
	size_t r = 0;
	if (k = getnode(n[1]); !k[0]) r += k[1]; else r += count(n[1]) << (k[0] - n[0] - 1);
	if (k = getnode(n[2]); !k[0]) return r + k[1]; else return r + (count(n[2])<<(k[0]-n[0]-1));
}

vbits bdds::allsat(int_t x) const { vbits r; return r.reserve(satcount(x)), sat(x, r); }
vbits& bdds::sat(int_t x, vbits& r) const {
	node n = getnode(x);
	node nl = getnode(n[1]), nr = getnode(n[2]);
	vbits s1, s2;
	if (nl[0]||nl[1]) { s1=r; for (int_t k=n[0]-1; k!=nl[0]; ++k) s1 *= { s1, k }; }
	if (nr[0]||nr[1]) { s2=r; for (int_t k=n[0]-1; k!=nr[0]; ++k) s2 *= { s2, k }; }
	return r = s1 *= { s2, n[0] };
}

int_t bdds::from_eq(int_t x, int_t y) {
	return bdd_or(	bdd_and(from_bit(x, true), from_bit(y, true)),
			bdd_and(from_bit(x, false),from_bit(y, false)));
}

void bdds::out(wostream& os, const node& n) const {
	if (!n[0]) os << (n[1] ? L'T' : L'F');
	else out(os << n[0] << L'?', getnode(n[1])), out(os << L':', getnode(n[2]));
}
////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename op_t> int_t bdds::apply(const bdds& b, int_t x, bdds& r, const op_t& op) { //unary
	node n = op(b, b.getnode(x));
	return r.add({n[0], n[1]>1?apply(b,n[1],r,op):n[1], n[2]>1?apply(b,n[2],r,op):n[2]});
}
template<typename op_t> int_t bdds::apply(bdds& b, int_t x, bdds& r, const op_t& op) { // nonconst
	node n = op(b, b.getnode(x));
	return r.add({n[0], n[1]>1?apply(b,n[1],r,op):n[1], n[2]>1?apply(b,n[2],r,op):n[2]});
}

int_t bdds::permute(bdds& b, int_t x, bdds& r, const map<int_t, int_t>& m) { // [overlapping] rename
	node n = b.getnode(x);
	if (!n[0]) return x;
	auto it = m.find(n[0]);
	if (it == m.end()) throw 0;
	return r.add({it->second, n[1]>1?permute(b,n[1],r,m):n[1], n[2]>1?permute(b,n[2],r,m):n[2]});
}

template<typename op_t> // binary application
int_t bdds::apply(const bdds& bx, int_t x, const bdds& by, int_t y, bdds& r, const op_t& op) {
	const node &Vx = bx.getnode(x), &Vy = by.getnode(y);
	const int_t &vx = Vx[0], &vy = Vy[0];
	int_t v, a = Vx[1], b = Vy[1], c = Vx[2], d = Vy[2];
	if ((!vx && vy) || (vy && (vx > vy))) a = c = x, v = vy;
	else if (!vx) return op(a, b);
	else if ((v = vx) < vy || !vy) b = d = y;
	return r.add({v, apply(bx, a, by, b, r, op), apply(bx, c, by, d, r, op)});
}
////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename K> rule bdds::from_rule(matrix<K> v, const size_t bits, const size_t ar) {
	int_t r = T, k;
	size_t i, j, b;
	map<K, array<size_t, 2>> m;
	set<K> ex;
	map<K, int_t> hvars;
	map<int_t, int_t> hv;
	bool neg = v[0][0] < 0, bneg; // negation denoted by negative relid
	if (neg) v[0][0] = -v[0][0];
	for (i = 0; i != v[0].size(); ++i) if (v[0][i] < 0) hvars.emplace(i, v[0][i]); // head vars
	for (i = 1; i != v.size(); ++i) { // go over bodies
		if (k = T; (bneg = (v[i][0] < 0))) v[i][0] = -v[i][0];
		for (j = 0; j != v[i].size(); ++j) // per relid/arg
			if (auto it = m.find(v[i][j]); it != m.end()) // if seen
				for (b = 0; b != bits; ++b)
					k = bdd_and(k,from_eq((i*bits+b)*ar+j,
							(it->second[0]*bits+b)*ar+it->second[1]));
			else if (m.emplace(v[i][j], array<size_t, 2>{ i, j }); v[i][j] > 0) // sym
				for (b = 0; b != bits; ++b)
					k = bdd_and(k, from_bit((i*bits+b)*ar+j, v[i][j]&(1<<b)));
			else if (auto jt = hvars.find(v[i][j]); jt == hvars.end()) //non-head var
				for (b = 0; b != bits; ++b)
					ex.emplace((i*bits+b)*ar+j); // is an "existential"
			else for (b = 0; b != bits; ++b)
				hv.emplace((i*bits+b)*ar+j, b * ar + jt->second);
		r = bneg ? bdd_and(r, k) : bdd_and_not(r, k);
	}
	return { neg, r, v.size()-1, ex, hv };
}

vbits& operator*=(vbits& x, const pair<const vbits&, size_t>& y) {
	size_t sx = x.size(), sy = y.first.size();
	x.reserve(sx + sy);
	for (size_t n = 0; n != sy; ++n) x.push_back(y.first[n]);
	sy += sx;
	while (sy-- != sx) x[sy][y.second] = false;
	while (sx--) x[sx][y.second] = true;
	return x;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename K>
bool dict_t<K>::dictcmp::operator()(const pair<wstr, size_t>& x, const pair<wstr, size_t>& y) const{
	if (x.second != y.second) return x.second < y.second;
	return wcsncmp(x.first, y.first, x.second) < 0;
}

template<typename K> K dict_t<K>::operator()(wstr s, size_t len) {
	if (*s == L'?') {
		if (auto it = vars_dict.find({s, len}); it != vars_dict.end())
			return it->second;
		return vars_dict[{s, len}] = -vars_dict.size();
	}
	if (auto it = syms_dict.find({s, len}); it != syms_dict.end()) return it->second;
	return syms.push_back(s), lens.push_back(len), syms.size();
}

template<typename K> K lp<K>::str_read(wstr *s) {
	wstr t;
	while (**s && iswspace(**s)) ++*s;
	if (!**s) return 0;
	if (*(t = *s) == L'?') ++t;
	while (iswalnum(*t)) ++t;
	while (iswspace(*t)) ++t;
	if (t == *s) return 0;
	K r = dict(*s, t - *s);
	while (*t && iswspace(*t)) ++t;
	return *s = t, r;
}

template<typename K> vector<K> lp<K>::term_read(wstr *s) {
	vector<K> r;
	while (iswspace(**s)) ++*s;
	bool b = **s == L'~';
	if (b) ++*s;
	K rel = str_read(s), t;
	r.push_back(b ? rel : -rel);
	if (!rel) return {};
	if (*((*s)++) != L'(') er(oparen_expected);
	do {
		while (iswspace(**s)) ++*s;
		if (**s == L')') return r;
		if (!(t = str_read(s))) er("identifier expected");
		r.push_back(t);
	} while (**s);
	er("term_read(): unexpected parsing error");
}

template<typename K> matrix<K> lp<K>::rule_read(wstr *s) {
	vector<K> t;
	matrix<K> r;
	if ((t = term_read(s)).empty()) return r;
	while (iswspace(**s)) ++*s;
	if (**s == L'.') return r;
	if (*((*s)++) != L':' || *((*s)++) != L'-') er(sep_expected);
loop:	if ((t = term_read(s)).empty()) er("term expected");
	while (iswspace(**s)) ++*s;
	if (**s == L'.') return r;
	goto loop;
}

template<typename K> void lp<K>::prog_read(wstr s) {
	vector<matrix<K>> r;
	int_t db = bdds::T;
	size_t ar = 0, l;
	for (matrix<K> t; !(t = rule_read(&s)).empty(); r.push_back(t))
		for (const vector<K>& x : t) // we really support a single rel arity
			ar = max(ar, x.size()); // so we'll pad everything
	for (matrix<K>& x : r)
		for (vector<K>& y : x)
			if ((l=y.size()) < ar)
				y.resize(ar), fill(y.begin()+l, y.end(), dict.pad); // the padding
	for (const matrix<K>& x : r)
		if (x.size() == 1) db = dbs.bdd_or(db, dbs.from_rule(x, dict.bits(), ar).h);// fact
		else rules.push_back(prog.from_rule(x, dict.bits(), ar)); // rule
}

template<typename K> void lp<K>::step() {
	int_t add = bdds::F, del = bdds::F, s;
	for (const rule& r : rules) { // per rule
		dbs.setpow(db, r.w);
		int_t x = bdds::apply(prog, r.h, dbs, db, prog, op_and); // rule/db conjunction
		int_t y = bdds::apply(prog, x, prog, op_exists(r.x)); // remove nonhead variables
		int_t z = bdds::permute(prog, y, prog, r.hvars); // reorder the remaining vars
		(r.neg ? del : add) = prog.bdd_or(r.neg ? del : add, z); // disjunct with add/del
	}
	if ((s = prog.bdd_and_not(add, del)) == bdds::F) db = bdds::F; // detect contradiction
	else db = prog.bdd_or(prog.bdd_and_not(bdds::T, del), s); // db = (db|add)&~del
}
////////////////////////////////////////////////////////////////////////////////////////////////////
wstring file_read_text(FILE *f) {
	wstringstream ss;
	wchar_t buf[32], n, l, skip = 0;
	wint_t c;
	*buf = 0;
next:	for (n = l = 0; n != 31; ++n)
		if (WEOF == (c = getwc(f))) { skip = 0; break; }
		else if (c == L'#') skip = 1;
		else if (c == L'\r' || c == L'\n') skip = 0, buf[l++] = c;
		else if (!skip) buf[l++] = c;
	if (n) {
		buf[l] = 0, ss << buf;
		goto next;
	} else if (skip) goto next;
	return ss.str();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
int main() {
	setlocale(LC_ALL, "");
	lp<int32_t> p;
	p.prog_read(file_read_text(stdin).c_str());
	p.step();
	//p.printdb(wcout);
	return 0;
}