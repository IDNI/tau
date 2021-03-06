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
#ifndef __DRIVER_H__
#define __DRIVER_H__
#include <map>
#include <cmath>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#endif
#include "tables.h"
#include "input.h"
#include "dict.h"
#include "output.h"
#include "options.h"


typedef enum prolog_dialect { XSB, SWIPL } prolog_dialect;
typedef std::map<elem, elem> var_subs;
typedef std::pair<std::set<raw_term>, var_subs> terms_hom;
typedef std::tuple<elem, int_t> rel_info;

#define QFACT 0
#define QRULE 1
#define QTERM 2
#define QEQUALS 3
#define QFORALL 4
#define QEXISTS 5
#define QNOT 6
#define QAND 7
#define QALT 8
#define QIMPLIES 9
#define QUNIQUE 10
#define QCOIMPLIES 11

class archive;

struct prog_data {
	strs_t strs;
//	std::unordered_map<int_t, term> strtrees;
//	std::vector<term> out;
//	matpairs r;
//	matrix goals, tgoals;
	bool bwd = false;
	size_t n = 0;
	size_t start_step = 0;
	size_t elapsed_steps = 0;
	string_t std_input;
};

class driver {
	friend class archive;
	friend struct flat_rules;
	friend struct pattern;
	friend std::ostream& operator<<(std::ostream& os, const driver& d);
	friend std::istream& operator>>(std::istream& is, driver& d);
	dict_t dict;
	std::set<int_t> builtin_rels;//, builtin_symbdds;

	int_t nums = 0, chars = 0, syms = 0;
//	bool mult = false;

	std::set<lexeme, lexcmp> strs_extra, rels;
	std::vector<ccs> strs_allocated;
	lexeme get_var_lexeme(int_t i);
	lexeme get_new_var();
	lexeme get_lexeme(ccs w, size_t l = (size_t)-1);
	lexeme get_lexeme(const std::basic_string<char>& s);
	lexeme get_lexeme(const std::basic_string<unsigned char>& s);
	lexeme get_new_rel();
//	std::function<int_t(void)> *fget_new_rel;
//	lexeme get_num_lexeme(int_t i);
//	lexeme get_char_lexeme(char_t i);
//	lexeme get_demand_lexeme(elem e, const ints& i, const bools& b);
	void refresh_vars(raw_term& t, size_t& v, std::map<elem, elem>& m);
	void refresh_vars(raw_prog& p);
	raw_rule refresh_vars(raw_term h, std::vector<std::vector<raw_term>> b);
	std::set<raw_rule> refresh_vars(raw_rule& r);
	std::set<raw_term> get_queries(const raw_prog& p);

	string_t directive_load(const directive& d);
	void directives_load(raw_prog& p, lexeme& trel,
		const raw_term &false_term);
	bool transform(raw_prog& rp, const strs_t& strtrees);
//	std::set<raw_rule> transform_ms(const std::set<raw_rule>& p,
//		const std::set<raw_term>& qs);
//	raw_prog transform_sdt(const raw_prog& p);
	void transform_bin(raw_prog& p);
	void transform_len(raw_term& r, const strs_t& s);
//	raw_prog transform_bwd(raw_prog& p);
	raw_term get_try_pred(const raw_term& x);
	void transform_bwd(const raw_term& h, const std::vector<raw_term>& b,
		std::set<raw_rule>& s);
	void transform_bwd(raw_prog& p);
	void transform_proofs(raw_prog& r, const lexeme& rel);
//	void transform_string(const sysstring_t&, raw_prog&, int_t);
	void transform_grammar(raw_prog& r, lexeme rel, size_t len);
	bool transform_evals(raw_prog &rp, const directive &drt);
	bool transform_quotes(raw_prog &rp, const raw_term &false_term,
		const directive &drt);
	bool transform_domains(raw_prog &rp, const directive& drt);
	bool transform_codecs(raw_prog &rp, const directive &drt);
	sprawformtree expand_formula_node(const sprawformtree &t);
	void flatten_associative(const elem::etype &tp,
		const sprawformtree &tree, std::vector<sprawformtree> &tms);
	bool is_cq(const raw_rule &rr);
	bool is_cqn(const raw_rule &rr);
	template<typename F> bool try_minimize(raw_rule &rr, const F &f);
	int_t count_related_rules(const raw_rule &rr1, const raw_prog &rp);
	void step_transform(raw_prog &rp,
		const std::function<void(raw_prog &)> &f);
	void recursive_transform(raw_prog &rp,
		const std::function<void(raw_prog &)> &f);
	raw_rule freeze_rule(raw_rule rr, std::map<elem, elem> &freeze_map,
		dict_t &d);
	bool cqc(const raw_rule &rr1, const raw_rule &rr2);
	bool cqnc(const raw_rule &rr1, const raw_rule &rr2);
	bool cbc(const raw_rule &rr1, raw_rule rr2, std::set<terms_hom> &homs);
	void eliminate_dead_variables(raw_prog &rp);
	void factor_rules(raw_prog &rp);
	raw_prog read_prog(elem prog, const raw_prog &rp);
	void simplify_formulas(raw_prog &rp, const raw_term &false_term);
	elem quote_elem(const elem &e, std::map<elem, elem> &variables,
		dict_t &d);
	elem numeric_quote_elem(const elem &e, std::map<elem, elem> &variables);
	elem quote_term(const raw_term &head, const elem &rel_name,
		const elem &domain_name, raw_prog &rp, std::map<elem, elem> &variables,
		int_t &part_count);
	elem quote_formula(const sprawformtree &t, const elem &rel_name,
		const elem &domain_name, raw_prog &rp, std::map<elem, elem> &variables,
		int_t &part_count);
	std::vector<elem> quote_rule(const raw_rule &rr, const elem &rel_name,
		const elem &domain_name, raw_prog &rp, int_t &part_count,
		const raw_term &false_term);
	void quote_prog(const raw_prog nrp, const elem &rel_name,
		const elem &domain_name, raw_prog &rp, const raw_term &false_term);
	raw_term to_pure_tml(const sprawformtree &t, raw_prog &rp,
		const std::set<elem> &fv);
	void collect_vars(const raw_rule &rr, std::set<elem> &vars);
	void collect_vars(const raw_term &rt, std::set<elem> &vars);
	template <class InputIterator>
		void collect_vars(InputIterator first, InputIterator last,
			std::set<elem> &vars);
	void to_pure_tml(raw_prog &rp);
	void compute_required_vars(const raw_rule &rr, const terms_hom &hom,
		std::set<elem> &orig_vars);
	void collect_free_vars(const std::vector<std::vector<raw_term>> &b,
		std::vector<elem> &bound_vars, std::set<elem> &free_vars);
	void collect_free_vars(const raw_rule &rr, std::set<elem> &free_vars);
	std::set<elem> collect_free_vars(const raw_rule &rr);
	void collect_free_vars(const raw_term &t,
		const std::vector<elem> &bound_vars, std::set<elem> &free_vars);
	std::set<elem> collect_free_vars(const raw_term &t);
	void collect_free_vars(const sprawformtree &t,
		std::vector<elem> &bound_vars, std::set<elem> &free_vars);
	std::set<elem> collect_free_vars(const std::vector<std::vector<raw_term>> &b);
	std::set<elem> collect_free_vars(const sprawformtree &t);
	raw_term relation_to_term(const rel_info &ri);
	bool transform_grammar(raw_prog &rp);
	void remove_redundant_exists(raw_prog &rp);
	sprawformtree fix_variables(const elem &fv_rel, const elem &qva,
		const elem &rva, const elem &qvb, const elem &rvb);
	sprawformtree fix_symbols(const elem &fs_rel, const elem &qva,
		const elem &rva);
	template<typename F> void subsume_queries(raw_prog &rp, const F &f);
	elem concat(const elem &rel, std::string suffix);
	lexeme concat(const lexeme &rel, std::string suffix);
	string_t generate_cpp(const elem &e, string_t &prog_constr, uint_t &cid,
		const string_t &dict_name, std::map<elem, string_t> &elem_cache);
	string_t generate_cpp(const raw_term &rt, string_t &prog_constr, uint_t &cid,
		const string_t &dict_name, std::map<elem, string_t> &elem_cache);
	string_t generate_cpp(const sprawformtree &prft, string_t &prog_constr,
		uint_t &cid, const string_t &dict_name, std::map<elem, string_t> &elem_cache);
	string_t generate_cpp(const raw_rule &rr, string_t &prog_constr, uint_t &cid,
		const string_t &dict_name, std::map<elem, string_t> &elem_cache,
		const raw_term &false_term);
	string_t generate_cpp(const raw_prog &rp, string_t &prog_constr, uint_t &cid,
		const string_t &dict_name, std::map<elem, string_t> &elem_cache,
		const raw_term &false_term);
	raw_prog reify(const raw_prog& p);
	raw_term from_grammar_elem(const elem& v, int_t v1, int_t v2);
	raw_term from_grammar_elem_nt(const lexeme& r, const elem& c,
		int_t v1, int_t v2);
	raw_term from_grammar_elem_builtin(const lexeme& r, const string_t&b,
		int_t v);
	raw_term prepend_arg(const raw_term& t, lexeme s);
//	template <typename T>
//	void get_trees(std::basic_ostream<T>& os, const term& root,
//		const std::map<term, std::vector<term>>&, std::set<term>& done);
//	sysstring_t get_trees(const term& roots,const db_t& t,size_t bits);
	void progs_read(cstr s);
	bool prog_run(raw_prog& rp, nlevel steps=0, size_t brstep=0);
	//driver(raw_progs, options o);
	//driver(raw_progs);
	size_t load_stdin();
	prog_data pd;
	std::set<int_t> transformed_strings;
	tables *tbl = 0;
	ir_builder *ir = 0;

	void output_pl(const raw_prog& p) const;
	std::set<lexeme> vars;
	raw_progs rp;
	bool running = false;
	inputs* ii;
	inputs dynii; // For inputs generated from running TML programs
	input* current_input = 0;
	size_t current_input_id = 0;
	std::vector<archive> load_archives;
public:
	bool result = false;
	bool error = false;
	options opts;
	driver(const options& o);
	driver(FILE *f, const options& o);
	driver(string_t, const options& o);
	driver(std::string, const options& o);
	driver(const char *s, const options& o);
	driver(ccs s, const options& o);
	driver(FILE *f);
	driver(std::string);
	driver(string_t);
	driver(const char *s);
	driver(ccs s);
	~driver();

	template <typename T>
	void info(std::basic_ostream<T>&);
	template <typename T>
	void list(std::basic_ostream<T>& os, size_t p = 0);
	bool add(input* in);
	void restart();
	bool run(size_t steps = 0, size_t br_on_step=0);
	bool step(size_t steps = 1, size_t br_on_step=0);
	size_t nsteps() { return tbl->step(); };
	template <typename T>
	void out(std::basic_ostream<T>& os) const { if (tbl) tbl->out(os); }
	void dump() { out(o::dump()); }
	template <typename T>
	void out_fixpoint(std::basic_ostream<T>& os) const { if (tbl) tbl->out_fixpoint(os); }
	void dump_fixpoint() { out_fixpoint(o::dump()); }
	void out(const tables::rt_printer& p) const { if (tbl) tbl->out(p); }
	void set_print_step   (bool val) { tbl->print_steps   = val; }
	void set_print_updates(bool val) { tbl->print_updates = val; }
	void set_populate_tml_update(bool val) { tbl->populate_tml_update=val; }
	void set_regex_level(int val ) { ir->regex_level = val; }
	template <typename T>
	bool out_goals(std::basic_ostream<T>& os) const {
		return tbl->get_goals(os); }
	template <typename T>
	void out_dict(std::basic_ostream<T>& os) const { tbl->print_dict(os); }
	size_t size();
	void load(std::string filename);
	void save(std::string filename);
	size_t db_size();
	void db_load(std::string filename);
	void db_save(std::string filename);
	inputs* get_inputs() const { return ii; }
	input* get_current_input() const { return current_input; }
	void set_current_input(input* in) { current_input = in; }
	void read_inputs();
	
#ifdef __EMSCRIPTEN__
	void out(emscripten::val o) const { if (tbl) tbl->out(o); }
	emscripten::val to_bin() {
		std::stringstream ss; ss << (*this);
		std::string bin = ss.str();
		emscripten::val r = emscripten::val::global("Uint8Array")
							.new_(bin.size());
		int n = 0;
		for (unsigned char b : bin) r.set(n++, b);
		return r;
	}
#endif

//	std::basic_ostream<T>& printbdd(std::basic_ostream<T>& os, spbdd t, size_t bits,
//		const prefix&) const;
//	std::basic_ostream<T>& printbdd_one(std::basic_ostream<T>& os, spbdd t, size_t bits,
//		const prefix&) const;
	template <typename T>
	std::basic_ostream<T>& print_prolog(std::basic_ostream<T>& os,
		const raw_prog&, prolog_dialect) const;
	template <typename T>
	std::basic_ostream<T>& print_xsb(std::basic_ostream<T>& os,
		const raw_prog&) const;
	template <typename T>
	std::basic_ostream<T>& print_swipl(std::basic_ostream<T>& os,
		const raw_prog&) const;
	template <typename T>
	std::basic_ostream<T>& print_souffle(std::basic_ostream<T>& os,
		const raw_prog&) const;
	void save_csv() const;
};

template void driver::out<char>(std::ostream&) const;
template void driver::out<wchar_t>(std::wostream&) const;
template bool driver::out_goals(std::basic_ostream<char>&) const;
template bool driver::out_goals(std::basic_ostream<wchar_t>&) const;
template void driver::out_dict(std::basic_ostream<char>&) const;
template void driver::out_dict(std::basic_ostream<wchar_t>&) const;

#ifdef DEBUG
extern driver* drv;
//template <typename T>
//std::basic_ostream<T>& printdb(std::basic_ostream<T>& os, lp *p);
template <typename T>
std::basic_ostream<T>& printbdd(std::basic_ostream<T>& os, cr_spbdd_handle t,
	size_t bits, ints ar, int_t rel);
template <typename T>
std::basic_ostream<T>& printbdd_one(std::basic_ostream<T>&os, cr_spbdd_handle t,
	size_t bits, ints ar, int_t rel);
//template <typename T>
//std::basic_ostream<T>& printbdd(std::basic_ostream<T>& os, size_t t, size_t bits, ints ar,
//	int_t rel);
#endif
string_t _unquote(string_t str);
#endif
