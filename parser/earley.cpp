#include <vector>
#include <map>
#include <set>
#include <array>
#include <string>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <climits>
#include <sstream>
using namespace std;

typedef array<size_t, 2> coord;
typedef array<size_t, 2> interval;
enum gitem_t { NULLABLE, TERMINAL, NONTERM };
#define subset(x, y) includes(x.begin(), x.end(), y.begin(), y.end())

struct gitem {
	gitem_t t;
	union { wchar_t ch; interval i; };
	void set(wchar_t s) { t = TERMINAL; ch = s; }
	void set(interval in, bool n) { i = in; t = n ? NULLABLE : NONTERM; }
}; 
typedef pair<vector<vector<gitem>>, map<size_t, const wchar_t*>> compiled_cfg;

wstring format(const compiled_cfg& g, size_t i, size_t j) {
	wstringstream ss;
	gitem t;
	if (j < g.first[i].size()) t = g.first[i][j];
	for (size_t n = 0; n < g.first[i].size(); ++n) {
		if (n == j) ss << "* ";
		if (j < g.first[i].size()) {
			if (t.t==TERMINAL) ss << t.ch;
			else ss << g.second.at(t.i[0]) << ' ';
		}
	}
	return ss.str();
}

compiled_cfg cfg_compile(vector<vector<wstring>> g, wstring S) {
	compiled_cfg r;
	set<wstring> nulls;
	map<wstring, interval> alts;
	map<wstring, set<wstring>> in, out;
	map<wstring, set<wstring>>::const_iterator it, jt;
	map<wstring, interval>::const_iterator kt;
	wstring Z = L"Z";
	size_t s, i = 0, j = 0;

	if (!is_sorted(g.begin(), g.end())) throw 0;
	nulls.emplace(), sort(g.begin(), g.end()), r.first.resize(g.size() + 1);
	while (Z <= g[g.size()-1][0]) Z += L'Z';
	g.push_back({Z, S});
	//wcout<<g.size()<<endl;
	for (bool b = false; !b; ++i)
		if ((b = (i == g.size() - 1)) || g[i][0] != g[i+1][0])
			alts.emplace(g[i][0], interval{{j, i + 1}}), j = i + 1;
	for (auto a : alts) r.second.emplace(a.second[0], wcsdup(a.first.c_str()));
	for (i = 0; i < g.size(); ++i)
		for (r.first[i].resize(g[i].size() - 1), j = 1; j < g[i].size(); ++j)
			out[g[i][0]].emplace(g[i][j]),
			in[g[i][j]].emplace(g[i][0]);
iter:	s = r.first.size();
	for (const wstring& t : nulls)
		if ((it = in.find(t)) == in.end()) continue;
		else for (const wstring& y : it->second)
			if (	(jt = out.find(y)) != out.end() &&
				subset(jt->second, nulls)) nulls.emplace(y);
	if (s != r.first.size()) goto iter;
	for (i = 0; i < g.size(); ++i)
		for (j = 1; j < g[i].size(); ++j)
			if ((kt = alts.find(g[i][j])) != alts.end())
				r.first[i][j-1].set(kt->second,
					nulls.find(g[i][j]) != nulls.end());
			else r.first[i][j-1].set(g[i][j][0]);
	return r;
}

struct eitem {
	size_t end, nt, len, alt, dot;
	eitem(	size_t end = 0, size_t nt = 0, size_t len = 0, size_t alt = 0,
		size_t dot = 0):end(end),nt(nt),len(len),alt(alt),dot(dot) {}
	eitem(const vector<vector<gitem>>& g, size_t end, size_t len,
		size_t alt, size_t dot):eitem(end, g[alt].size()==dot? UINT_MAX:
			g[alt][dot].t != TERMINAL ? g[alt][dot].i[0] : UINT_MAX
			, len, alt, dot) {}
};

#define cmp_(t) x.t!=y.t
#define cmp(t) cmp_(t)?x.t<y.t
bool operator<(const eitem& x, const eitem& y) {
	return cmp(end):cmp(nt):cmp(len):cmp(alt):cmp(dot):false; }
bool operator!=(const eitem& x, const eitem& y) {
	return cmp_(end)||cmp_(nt)||cmp_(len)||cmp_(alt)||cmp_(dot); }

wstring format(const compiled_cfg& g, eitem i) {
	wstringstream ss;
	ss << '[' << i.end-i.len << ':' << i.end << "] " << format(g, i.alt, i.dot);
	if (i.nt != UINT_MAX) ss << " (" << g.second.at(i.nt) << ')';
	return ss.str();
}

bool cfg_parse(const compiled_cfg& G, const wchar_t* in) {
#define add_item(i, j) outs[i].emplace(j), ins[j].emplace(i), front.emplace(j)
	auto &g = G.first;
	eitem i(g, 0, 0, g.size() - 1, 0), j, k;
	set<eitem> front;
	map<eitem, set<eitem>> ins, outs;
	set<eitem> s;
	map<eitem, set<eitem>>::const_iterator it, jt;
	gitem x;

start:	wcout << format(G, i) << endl;
	if (i.nt != UINT_MAX) {
		for (it = outs.lower_bound(eitem(i.end - i.len, i.nt, 0, 0, 0));
			it != outs.end() && it->first.end == i.end - i.len &&
			it->first.nt == i.nt; ++it)
			k = it->first,
			j = eitem(g, k.end, k.len+i.len, k.alt, k.dot + 1),
			add_item(i, k), add_item(k, j);
		goto cont;
	}
	x = g[i.alt][i.dot];
	switch (x.t) {
	case NULLABLE:	j = eitem(g,i.end, i.len, i.alt, i.dot + 1), add_item(i, j);
	case NONTERM :	for (size_t n = x.i[0], k = x.i[1]; n != k; ++n)
				j = eitem(g,i.end, 0, n, 0), add_item(i, j);
			break;
	case TERMINAL:	if (in[i.end] == x.ch)
				j = eitem(g,i.end+1, i.len+1, i.alt, i.dot+1),
				add_item(i, j);
			else goto gc;
	}

cont:	if (front.empty()) return false;
	i = *front.begin(), front.erase(front.begin());
	if (!front.empty() || i!=eitem(g,0, 0, g.size()-1, g[g.size()-1].size()))
		goto start;
	return true;

gc:	size_t sz = s.size();
	s.emplace(i);
	for (const eitem& t : s)
		if ((it = ins.find(t)) == ins.end()) continue;
		else for (const eitem& y : it->second)
			if ((jt=outs.find(y))!=outs.end()&&subset(jt->second,s))
				s.emplace(y);
	if (sz != s.size()) goto gc;
	for (eitem x : s) {
		if ((it = outs.find(x)) != outs.end())
			for (const eitem& ox : it->second) ins[ox].erase(x);
		if ((it = ins.find(x)) != ins.end())
			for (const eitem& ix : it->second) outs[ix].erase(x);
		ins.erase(x), outs.erase(x), front.erase(x);
	}
	s.clear();
	goto cont;
}

int main(int, char**) {
	setlocale(LC_ALL, "");
	assert(cfg_parse(cfg_compile({{L"S",L"a"}},L"S"),L"a"));
	assert(!cfg_parse(cfg_compile({{L"S",L"a"}},L"S"),L"aa"));
	return 0;
}