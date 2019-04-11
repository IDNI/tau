// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#include <sstream>
#include "lp.h"
#ifdef DEBUG
#include "driver.h"
#endif
using namespace std;

bool operator<(const db_t::const_iterator& x, const db_t::const_iterator& y) {
	return	x->first != y->first ? x->first < y->first :
		x->second != y->second ? x->second < y->second : false;
}

bool operator<(const set<term>::const_iterator& x,
	const set<term>::const_iterator& y) { return *x < *y; }

bool ar_prefix(ints x, ints y) {
	if (!x[0]) x.erase(x.begin());
	if (!y[0]) y.erase(y.begin());
	if (y.size() < x.size()) return false;
	for (size_t n = 0; n < x.size(); ++n) if (x[n] != y[n]) return false;
	return true;
}

set<db_t::const_iterator> lp::tree_prefix(const prefix& p) const {
	set<db_t::const_iterator> r;
	for (auto it = db.lower_bound({p.rel,{}}); it->first.rel == p.rel; ++it)
		if (ar_prefix(p.ar, it->first.ar)) r.insert(it);
	return r;
}

void lp::get_tree(const prefix& p, size_t root, set<size_t>& done) {
	if (!done.emplace(root).second) return;
	diff_t::iterator it;
	for (const pair<ints, array<size_t, 2>>& x : p.subterms())
		for (const db_t::const_iterator& y:tree_prefix({p.rel,x.first}))
			it = trees_out.emplace(y->first,F).first,
			get_tree(y->first, it->second = bdd_or(it->second,
				bdd_and(*y->second, bdd_subterm(root,
				x.second[0], x.second[1], p.len(),
				y->first.len(), rng.bits))), done);
}

void lp::get_trees() {
	set<size_t> done;
	for (auto x : trees)
		for (const db_t::const_iterator& it : tree_prefix(x.first))
			get_tree(it->first, bdd_and(*it->second, bdd_expand(
				x.second, x.first.len(), it->first.len(),
				rng.bits)), done), done.clear();
	db.clear();
	auto it = db.end();
	for (auto x : trees_out) {
		if ((it = db.find(x.first)) == db.end())
			it = db.emplace(x.first, new size_t).first;
		*it->second = x.second;
	}
}

set<set<term>::const_iterator> tree_prefix(const term& t, const set<term>& s) {
	set<set<term>::const_iterator> r;
	for (auto it = s.lower_bound(t); it->root() == t; ++it) r.insert(it);
	return r;
}

void driver::get_trees(wostream& os, const term& root, const set<term>& s,
	set<term>& done) {
	if (!done.emplace(root).second) return;
	print_term(os, root);
	for (auto x : root.subterms())
		for (auto y : tree_prefix(x, s))
			get_trees(os, *y, s, done);
}

void driver::get_trees(const set<term>& roots, const diff_t& t, size_t bits) {
	set<term> m, done;
	for (auto x : t)
		from_bits(x.second, bits, x.first, [&m](const term& t) {
				m.insert(t); });
	wstringstream ss;
	for (const term& x : roots) get_trees(ss, x, m, done);
	wcout << L"get_trees: " << ss.str() << endl;
}
