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
#ifndef __ADDBITS_H__
#define __ADDBITS_H__
#include <map>
#include <vector>
#include "bdd.h"
#include "term.h"
#include "bitsmeta.h"
#include "dict.h"
#include "defs.h"
#include "types.h"
class tables;
struct alt;

struct AddBits {
	tables& rtables;
	std::set<tbl_arg> rdone;
	std::map<tbl_arg, perminfo> tblargperms;
	std::map<alt_arg, perminfo> altperms;
	std::map<std::pair<alt*, size_t>, alt_arg> altdone;
	std::set<std::pair<tbl_arg, alt*>> bodydone;
	size_t target_bits;
	base_type target_type;

	AddBits(tables& tbls) :rtables(tbls) {}

	void clear() {
		altperms.clear();
		tblargperms.clear();
		altdone.clear();
		rdone.clear();
		bodydone.clear();
	}

	alt* get_alt(const tbl_alt& talt) const;
	void permute_type(
		const tbl_arg& intype, size_t nbits = 1, bool bitsready = false);
	bits_perm permute_table(const tbl_arg& targ, size_t nbits, bool bitsready);
	bool permute_bodies(const bits_perm& p, alt& a); // , size_t nbits);

	static xperm permex_add_bit(ints poss, c_bitsmeta& bm, c_bitsmeta& altbm);
	static perminfo add_bit_perm(const bitsmeta& bm, 
		size_t arg, size_t args, size_t nbits, bool bitsready = false);
	static perminfo add_bit_perm(const bitsmeta& oldbm, const bitsmeta& newbm,
		size_t arg, size_t args, size_t nbits);
	static spbdd_handle add_bit(spbdd_handle h,
		const perminfo& perm, size_t arg, size_t args, size_t nbits);
	static spbdd_handle add_bit(spbdd_handle h, const bits_perm& p) {
		return add_bit(h, p.perm, p.arg, p.args, p.nbits);
	}
	//static spbdd_handle add_bit(
	//	spbdd_handle h, const bits_perm& p, size_t nbits) {
	//	return add_bit(h, p.perm, p.arg, p.args, nbits);
	//}
	
};

#endif // __ADDBITS_H__