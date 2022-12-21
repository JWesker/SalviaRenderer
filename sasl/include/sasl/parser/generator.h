#pragma once

#include <eflib/platform/config.h>

#include <sasl/common/token.h>

#include <any>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace sasl {
namespace common {
class diag_chat;
}
} // namespace sasl

namespace sasl::parser {

using token = sasl::common::token;

typedef std::vector<token> token_seq;
typedef token_seq::iterator token_iterator;

class attribute_visitor {};

class parser;
class expectation_failure : public std::exception {
public:
  expectation_failure(token_iterator iter, parser const *p);
  parser const *get_parser();
  virtual const char *what() const throw();

#if defined(EFLIB_MINGW) || defined(EFLIB_GCC)
  virtual ~expectation_failure() _GLIBCXX_USE_NOEXCEPT {}
#endif

private:
  token_iterator iter;
  parser const *p;
  std::string what_str;
};

//////////////////////////////////////////////////////////////////////////
// Attributes is generated by parser. They are organized as a tree.
class attribute {
public:
  attribute();
  virtual ~attribute();

  std::shared_ptr<attribute> child(size_t idx) const { return child(static_cast<int>(idx)); }

  virtual std::shared_ptr<attribute> child(int idx) const = 0;

  virtual size_t child_size() const = 0;

  virtual intptr_t rule_id() const;
  virtual void rule_id(intptr_t id);

  virtual token token_beg() const;
  virtual token token_end() const;
  
  virtual void token_range(token, token = token::uninitialized());

protected:
  intptr_t rid;
  token tok_beg;
  token tok_end;
};

// Terminal
class terminal_attribute : public attribute {
public:
  virtual std::shared_ptr<attribute> child(int idx) const;
  virtual size_t child_size() const;
  token tok = token::uninitialized();
};

// *rule
// +rule
// -rule
class sequence_attribute : public attribute {
public:
  virtual std::shared_ptr<attribute> child(int idx) const;
  virtual size_t child_size() const;
  std::vector<std::shared_ptr<attribute>> attrs;
};

// rule0 | rule1
class selector_attribute : public attribute {
public:
  selector_attribute();

  virtual std::shared_ptr<attribute> child(int idx) const;
  virtual size_t child_size() const;
  std::shared_ptr<attribute> attr;
  int selected_idx;
};

// rule0 >> rule1
// rule0 > rule1
class queuer_attribute : public attribute {
public:
  virtual std::shared_ptr<attribute> child(int idx) const;
  virtual size_t child_size() const;
  std::vector<std::shared_ptr<attribute>> attrs;
};

//////////////////////////////////////////////////////////////////////////
// Parser combinators.

class parse_results {
public:
  parse_results();
  explicit parse_results(int i);

  static parse_results const succeed;
  static parse_results const recovered;
  static parse_results const recovered_expected_failed;
  static parse_results const failed;
  static parse_results const expected_failed;

  static parse_results recover(parse_results const &v);
  static parse_results worse(parse_results const &l, parse_results const &r);
  static parse_results better(parse_results const &l, parse_results const &r);

  bool worse_than(parse_results const &v) const;
  bool better_than(parse_results const &v) const;

  bool is_succeed() const;
  bool is_failed() const;
  bool is_recovered() const;
  bool is_expected_failed() const;
  bool is_recovered_expected_failed() const;

  bool is_expected_failed_or_recovered() const;
  bool is_continuable() const;

private:
  int tag;
};

typedef std::function<parse_results(sasl::common::diag_chat * /*diags*/,
                                    token_iterator const & /*origin iter*/,
                                    token_iterator & /*current start iter*/)>
    error_handler;
class error_catcher;

class parser {
public:
  parser();
  virtual parse_results parse(token_iterator &iter, token_iterator end,
                              std::shared_ptr<attribute> &attr,
                              sasl::common::diag_chat *diags) const = 0;
  bool is_expected() const;
  void is_expected(bool v);
  error_catcher operator[](error_handler on_err);
  virtual std::shared_ptr<parser> clone() const = 0;
  virtual ~parser() {}

private:
  bool expected;
};

class terminal : public parser {
public:
  terminal(size_t tok_id, std::string const &desc);
  terminal(terminal const &rhs);
  parse_results parse(token_iterator &iter, token_iterator end, std::shared_ptr<attribute> &attr,
                      sasl::common::diag_chat *diags) const;
  std::shared_ptr<parser> clone() const;
  std::string const &get_desc() const;

private:
  terminal &operator=(terminal const &);
  size_t tok_id;
  std::string desc;
};

class repeater : public parser {

public:
  static size_t const unlimited;

  repeater(size_t lower_bound, size_t upper_bound, std::shared_ptr<parser> expr);
  repeater(repeater const &rhs);
  parse_results parse(token_iterator &iter, token_iterator end, std::shared_ptr<attribute> &attr,
                      sasl::common::diag_chat *diags) const;
  std::shared_ptr<parser> clone() const;

private:
  size_t lower_bound;
  size_t upper_bound;
  std::shared_ptr<parser> expr;
};

class selector : public parser {

public:
  selector();
  selector(selector const &rhs);

  selector &add_branch(std::shared_ptr<parser> p);
  std::vector<std::shared_ptr<parser>> const &branches() const;

  parse_results parse(token_iterator &iter, token_iterator end, std::shared_ptr<attribute> &attr,
                      sasl::common::diag_chat *diags) const;
  std::shared_ptr<parser> clone() const;

private:
  std::vector<std::shared_ptr<parser>> slc_branches;
};

class queuer : public parser {
public:
  queuer();
  queuer(queuer const &rhs);

  queuer &append(std::shared_ptr<parser> p, bool is_expected = false);
  std::vector<std::shared_ptr<parser>> const &exprs() const;

  parse_results parse(token_iterator &iter, token_iterator end, std::shared_ptr<attribute> &attr,
                      sasl::common::diag_chat *diags) const;
  std::shared_ptr<parser> clone() const;

private:
  std::vector<std::shared_ptr<parser>> exprlst;
};

class negnativer : public parser {
public:
  negnativer(std::shared_ptr<parser>);
  negnativer(negnativer const &rhs);

  parse_results parse(token_iterator &iter, token_iterator end, std::shared_ptr<attribute> &attr,
                      sasl::common::diag_chat *diags) const;
  std::shared_ptr<parser> clone() const;

private:
  std::shared_ptr<parser> expr;
};

class rule : public parser {
public:
  rule();
  rule(intptr_t id);
  rule(std::shared_ptr<parser> expr, intptr_t id = -1);
  rule(rule const &rhs);
  rule(parser const &rhs);
  rule &operator=(parser const &rhs);
  rule &operator=(rule const &rhs);

  intptr_t id() const;
  std::string const &name() const;
  void name(std::string const &v);
  parser const *get_parser() const;
  parse_results parse(token_iterator &iter, token_iterator end, std::shared_ptr<attribute> &attr,
                      sasl::common::diag_chat *diags) const;
  std::shared_ptr<parser> clone() const;

private:
  intptr_t preset_id;
  std::shared_ptr<parser> expr;
  std::string rule_name;
};

class rule_wrapper : public parser {
public:
  rule_wrapper(rule_wrapper const &rhs);
  rule_wrapper(rule const &rhs);
  parse_results parse(token_iterator &iter, token_iterator end, std::shared_ptr<attribute> &attr,
                      sasl::common::diag_chat *diags) const;
  std::shared_ptr<parser> clone() const;
  std::string const &name() const;
  rule const *get_rule() const;

private:
  rule_wrapper &operator=(rule_wrapper const &);
  rule const &r;
};

class endholder : public parser {
public:
  endholder();
  endholder(endholder const &);
  parse_results parse(token_iterator &iter, token_iterator end, std::shared_ptr<attribute> &attr,
                      sasl::common::diag_chat *diags) const;
  std::shared_ptr<parser> clone() const;
};

class error_catcher : public parser {
public:
  error_catcher(std::shared_ptr<parser> const &p, error_handler err_handler);
  error_catcher(error_catcher const &);
  std::shared_ptr<parser> clone() const;
  parse_results parse(token_iterator &iter, token_iterator end, std::shared_ptr<attribute> &attr,
                      sasl::common::diag_chat *diags) const;

private:
  std::shared_ptr<parser> expr;
  error_handler err_handler;
};

// class exceptor: public parser
//{
// public:
//	exceptor( std::shared_ptr<parser> const& p );
//	exceptor( exceptor const& );
//	std::shared_ptr<parser> clone() const;
//	parse_results parse( token_iterator& iter, token_iterator end, std::shared_ptr<attribute>&
// attr, sasl::common::diag_chat* diags ) const; private: 	std::shared_ptr<parser>	expr;
// };
//////////////////////////////////////////////////////////////////////////
// Operators for building parser combinator.
repeater operator*(parser const &expr);
repeater operator-(parser const &expr);
selector operator|(parser const &expr0, parser const &expr1);
selector operator|(selector const &expr0, parser const &expr1);
selector operator|(selector const &expr0, selector const &expr1);
queuer operator>>(parser const &expr0, parser const &expr1);
queuer operator>>(queuer const &expr0, parser const &expr1);
queuer operator>(parser const &expr0, parser const &expr1);
queuer operator>(queuer const &expr0, parser const &expr1);
negnativer operator!(parser const &expr1);

} // namespace sasl::parser
