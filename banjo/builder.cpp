// Copyright (c) 2015-2016 Andrew Sutton
// All rights reserved

#include "builder.hpp"
#include "context.hpp"
#include "ast.hpp"

#include <unordered_set>


namespace banjo
{

// FIXME: Move this into lingo.
//
// A unique factory will only allocate new objects if they have not been
// previously created.
template<typename T, typename Hash, typename Eq>
struct Hashed_unique_factory : std::unordered_set<T, Hash, Eq>
{
  template<typename... Args>
  T& make(Args&&... args)
  {
    auto ins = this->emplace(std::forward<Args>(args)...);
    return *const_cast<T*>(&*ins.first); // Yuck.
  }
};


template<typename T>
struct Hash
{
  std::size_t operator()(T const& t) const
  {
    return hash_value(t);
  }

  std::size_t operator()(List<T> const& t) const
  {
    return hash_value(t);
  }
};


template<typename T>
struct Eq
{
  bool operator()(T const& a, T const& b) const
  {
    return is_equivalent(a, b);
  }

  bool operator()(List<T> const& a, List<T> const& b) const
  {
    return is_equivalent(a, b);
  }
};


template<typename T>
using Factory = Hashed_unique_factory<T, Hash<T>, Eq<T>>;


// -------------------------------------------------------------------------- //
// Builder definition

Symbol_table&
Builder::symbols() { return cxt.symbols(); }


// -------------------------------------------------------------------------- //
// Names

// Returns a simple identifier with the given spelling.
//
// TODO: Unique this?
Simple_id&
Builder::get_id(char const* s)
{
  Symbol const* sym = symbols().put_identifier(identifier_tok, s);
  return make<Simple_id>(*sym);
}


// Returns a simple identifier with the given spelling.
Simple_id&
Builder::get_id(std::string const& s)
{
  Symbol const* sym = symbols().put_identifier(identifier_tok, s);
  return make<Simple_id>(*sym);
}


// Returns a simple identifier for the given symbol.
Simple_id&
Builder::get_id(Symbol const& sym)
{
  lingo_assert(is<Identifier_sym>(&sym));
  return make<Simple_id>(sym);
}


// Returns a simple identifier for the symbol.
Simple_id&
Builder::get_id(Symbol const* sym)
{
  return get_id(*sym);
}


// Returns a simple id for the given token.
Simple_id&
Builder::get_id(Token tok)
{
  return get_id(tok.symbol());
}


// Returns a placeholder for a name.
//
// TODO: Make placeholders unique. Globally?
Placeholder_id&
Builder::get_id()
{
  return make<Placeholder_id>(cxt.get_unique_id());
}


Operator_id&
Builder::get_id(Operator_kind k)
{
  return make<Operator_id>(k);
}


// Returns a destructor-id for the given type.
Destructor_id&
Builder::get_destructor_id(Type const& t)
{
  lingo_unimplemented("destructor-id");
}


Template_id&
Builder::get_template_id(Template_decl& d, Term_list const& t)
{
  return make<Template_id>(d, t);
}


Concept_id&
Builder::get_concept_id(Concept_decl& d, Term_list const& t)
{
  return make<Concept_id>(d, t);
}


// Returns a qualified-id.
Qualified_id&
Builder::get_qualified_id(Decl& d, Name& n)
{
  return make<Qualified_id>(d, n);
}


// Return the global identifier.
Global_id&
Builder::get_global_id()
{
  // TODO: Global or no?
  static Global_id n;
  return n;
}


// -------------------------------------------------------------------------- //
// Types

User_type&
Builder::get_type(Type_decl& d)
{
  return make<User_type>(d);
}


Void_type&
Builder::get_void_type()
{
  return make<Void_type>();
}


Boolean_type&
Builder::get_bool_type()
{
  return make<Boolean_type>();
}


Integer_type&
Builder::get_integer_type(bool s, int p)
{
  return make<Integer_type>(s, p);
}

Byte_type&
Builder::get_byte_type()
{
  return make<Byte_type>();
}


// TODO: Default precision depends on configuration.
Integer_type&
Builder::get_int_type()
{
  return get_integer_type(true, 32);
}


// TODO: Default precision depends on configuration.
Integer_type&
Builder::get_uint_type()
{
  return get_integer_type(false, 32);
}


Float_type&
Builder::get_float_type()
{
  return make<Float_type>();
}


Auto_type&
Builder::get_auto_type()
{
  return make<Auto_type>();
}


Decltype_type&
Builder::get_decltype_type(Expr&)
{
  lingo_unimplemented("decltype-type");
}


Declauto_type&
Builder::get_declauto_type()
{
  return make<Declauto_type>();
}


Function_type&
Builder::get_function_type(Decl_list const& ps, Type& r)
{
  Type_list ts;
  for (Decl& d : *modify(&ps)) {
    Object_parm& p = cast<Object_parm>(d);
    ts.push_back(p.type());
  }
  return get_function_type(ts, r);
}


Function_type&
Builder::get_function_type(Type_list const& ts, Type& r)
{
  return make<Function_type>(ts, r);
}


// TODO: Do not build qualified types for functions or arrays.
// Is that a hard error, or do we simply fold the const into
// the return type and/or element type?
Qualified_type&
Builder::get_qualified_type(Type& t, Qualifier_set qual)
{
  if (Qualified_type* q = as<Qualified_type>(&t)) {
    q->qual |= qual;
    return *q;
  }
  return make<Qualified_type>(t, qual);
}


Qualified_type&
Builder::get_const_type(Type& t)
{
  return get_qualified_type(t, const_qual);
}


Qualified_type&
Builder::get_volatile_type(Type& t)
{
  return get_qualified_type(t, volatile_qual);
}


Pointer_type&
Builder::get_pointer_type(Type& t)
{
  return make<Pointer_type>(t);
}


Reference_type&
Builder::get_reference_type(Type& t)
{
  return make<Reference_type>(t);
}


Array_type&
Builder::get_array_type(Type&, Expr&)
{
  lingo_unimplemented("array-type");
}


Slice_type&
Builder::get_slice_type(Type& t)
{
  return make<Slice_type>(t);
}


Dynarray_type&
Builder::get_dynarray_type(Type&, Expr&)
{
  lingo_unimplemented("dynarray-type");
}


In_type&
Builder::get_in_type(Type& t)
{
  return make<In_type>(t);
}


Out_type&
Builder::get_out_type(Type& t)
{
  return make<Out_type>(t);
}


Mutable_type&
Builder::get_mutable_type(Type& t)
{
  return make<Mutable_type>(t);
}


Consume_type&
Builder::get_consume_type(Type& t)
{
  return make<Consume_type>(t);
}


Forward_type&
Builder::get_forward_type(Type& t)
{
  return make<Forward_type>(t);
}


Pack_type&
Builder::get_pack_type(Type& t)
{
  return make<Pack_type>(t);
}


Typename_type&
Builder::get_typename_type(Decl& d)
{
  return make<Typename_type>(d);
}


Type_type&
Builder::get_type_type()
{
  static Type_type t;
  return t;
}


Synthetic_type&
Builder::synthesize_type(Decl& d)
{
  return make<Synthetic_type>(d);
}


// -------------------------------------------------------------------------- //
// Expressions

Boolean_expr&
Builder::get_bool(bool b)
{
  return make<Boolean_expr>(get_bool_type(), b);
}


Boolean_expr&
Builder::get_true()
{
  return get_bool(true);
}


Boolean_expr&
Builder::get_false()
{
  return get_bool(false);
}


// TODO: Verify that T can have an integer value?
// I think that all scalars can have integer values.
Integer_expr&
Builder::get_integer(Type& t, Integer const& n)
{
  return make<Integer_expr>(t, n);
}


// Returns the 0 constant, with scalar type `t`.
//
// TODO: Verify that t is scalar.
//
// TODO: Produce zero interpratations for any T?
Integer_expr&
Builder::get_zero(Type& t)
{
  return get_integer(t, 0);
}


Integer_expr&
Builder::get_int(Integer const& n)
{
  return get_integer(get_int_type(), n);
}


Integer_expr&
Builder::get_uint(Integer const& n)
{
  // lingo_assert(n.is_nonnegative(n));
  return get_integer(get_uint_type(), n);
}


// Get an expression that refers to a variable. The type is a reference to 
// the declared type of the variable.
Object_expr&
Builder::make_reference(Variable_decl& d)
{
  Type& t = get_reference_type(d.type());
  Name& n = d.name();
  return make<Object_expr>(t, n, d);
}


// Get an expression that refers to a parameter. The type is a reference to 
// the declared type of the parameter.
Object_expr&
Builder::make_reference(Object_parm& d)
{
  Type& t = get_reference_type(d.type());
  Name& n = d.name();
  return make<Object_expr>(t, n, d);
}


// FIXME: Do I want functions to be references or not?
Function_expr&
Builder::make_reference(Function_decl& d)
{
  Type& t = get_reference_type(d.type());
  Name& n = d.name();
  return make<Function_expr>(t, n, d);
}


Field_expr&
Builder::make_member_reference(Expr& e, Field_decl& d)
{
  Type& t = get_reference_type(d.type());
  Name& n = d.name();
  return make<Field_expr>(t, e, n, d);
}


Method_expr&
Builder::make_member_reference(Expr& e, Method_decl& d)
{
  Type& t = get_reference_type(d.type());
  Name& n = d.name();
  return make<Method_expr>(t, e, n, d);
}


Member_expr&
Builder::make_member_reference(Expr& e, Overload_set& ovl)
{
  lingo_unimplemented("");
}


// Make a concept check. The type is bool.
Check_expr&
Builder::make_check(Concept_decl& d, Term_list const& as)
{
  return make<Check_expr>(get_bool_type(), d, as);
}


And_expr&
Builder::make_and(Type& t, Expr& e1, Expr& e2)
{
  return make<And_expr>(t, e1, e2);
}


Or_expr&
Builder::make_or(Type& t, Expr& e1, Expr& e2)
{
  return make<Or_expr>(t, e1, e2);
}


Not_expr&
Builder::make_not(Type& t, Expr& e)
{
  return make<Not_expr>(t, e);
}


Eq_expr&
Builder::make_eq(Type& t, Expr& e1, Expr& e2)
{
  return make<Eq_expr>(t, e1, e2);
}


Ne_expr&
Builder::make_ne(Type& t, Expr& e1, Expr& e2)
{
  return make<Ne_expr>(t, e1, e2);
}


Lt_expr&
Builder::make_lt(Type& t, Expr& e1, Expr& e2)
{
  return make<Lt_expr>(t, e1, e2);
}


Gt_expr&
Builder::make_gt(Type& t, Expr& e1, Expr& e2)
{
  return make<Gt_expr>(t, e1, e2);
}


Le_expr&
Builder::make_le(Type& t, Expr& e1, Expr& e2)
{
  return make<Le_expr>(t, e1, e2);
}


Ge_expr&
Builder::make_ge(Type& t, Expr& e1, Expr& e2)
{
  return make<Ge_expr>(t, e1, e2);
}


Add_expr&
Builder::make_add(Type& t, Expr& e1, Expr& e2)
{
  return make<Add_expr>(t, e1, e2);
}


Sub_expr&
Builder::make_sub(Type& t, Expr& e1, Expr& e2)
{
  return make<Sub_expr>(t, e1, e2);
}


Mul_expr&
Builder::make_mul(Type& t, Expr& e1, Expr& e2)
{
  return make<Mul_expr>(t, e1, e2);
}


Div_expr&
Builder::make_div(Type& t, Expr& e1, Expr& e2)
{
  return make<Div_expr>(t, e1, e2);
}


Rem_expr&
Builder::make_rem(Type& t, Expr& e1, Expr& e2)
{
  return make<Rem_expr>(t, e1, e2);
}


Neg_expr&
Builder::make_neg(Type& t, Expr& e)
{
  return make<Neg_expr>(t, e);
}


Pos_expr&
Builder::make_pos(Type& t, Expr& e)
{
  return make<Pos_expr>(t, e);
}


Bit_and_expr&
Builder::make_bit_and(Type& t, Expr& e1, Expr& e2)
{
  return make<Bit_and_expr>(t, e1, e2);
}


Bit_or_expr&
Builder::make_bit_or(Type& t, Expr& e1, Expr& e2)
{
  return make<Bit_or_expr>(t, e1, e2);
}


Bit_xor_expr&
Builder::make_bit_xor(Type& t, Expr& e1, Expr& e2)
{
  return make<Bit_xor_expr>(t, e1, e2);
}


Bit_lsh_expr&
Builder::make_bit_lsh(Type& t, Expr& e1, Expr& e2)
{
  return make<Bit_lsh_expr>(t, e1, e2);
}


Bit_rsh_expr&
Builder::make_bit_rsh(Type& t, Expr& e1, Expr& e2)
{
  return make<Bit_rsh_expr>(t, e1, e2);
}


Bit_not_expr&
Builder::make_bit_not(Type& t, Expr& e)
{
  return make<Bit_not_expr>(t, e);
}


Call_expr&
Builder::make_call(Type& t, Expr& f, Expr_list const& a)
{
  return make<Call_expr>(t, f, a);
}


Call_expr&
Builder::make_call(Type& t, Function_decl& f, Expr_list const& a)
{
  return make_call(t, make_reference(f), a);
}


Requires_expr&
Builder::make_requires(Decl_list const& tps, Decl_list const& ps, Req_list const& rs)
{
  return make<Requires_expr>(get_bool_type(), tps, ps, rs);
}


Synthetic_expr&
Builder::synthesize_expression(Decl& d)
{
  return make<Synthetic_expr>(declared_type(d), d);
}


// -------------------------------------------------------------------------- //
// Statements

Translation_stmt&
Builder::make_translation_statement(Stmt_list&& ss)
{
  return make<Translation_stmt>(std::move(ss));
}


Member_stmt&
Builder::make_member_statement(Stmt_list&& ss)
{
  return make<Member_stmt>(std::move(ss));
}


Compound_stmt&
Builder::make_compound_statement(Stmt_list&& ss)
{
  return make<Compound_stmt>(std::move(ss));
}


Empty_stmt&
Builder::make_empty_statement()
{
  return make<Empty_stmt>();
}


Return_stmt&
Builder::make_return_statement(Expr& e)
{
  return make<Return_stmt>(e);
}


If_then_stmt&
Builder::make_if_statement(Expr& e, Stmt& s)
{
  return make<If_then_stmt>(e, s);
}


If_else_stmt&
Builder::make_if_statement(Expr& e, Stmt& s1, Stmt& s2)
{
  return make<If_else_stmt>(e, s1, s2);
}


While_stmt&
Builder::make_while_statement(Expr& e, Stmt& s)
{
  return make<While_stmt>(e, s);
}


Break_stmt&
Builder::make_break_statement()
{
  return make<Break_stmt>();
}


Continue_stmt&
Builder::make_continue_statement()
{
  return make<Continue_stmt>();
}


Expression_stmt&
Builder::make_expression_statement(Expr& e)
{
  return make<Expression_stmt>(e);
}


Declaration_stmt&
Builder::make_declaration_statement(Decl& d)
{
  return make<Declaration_stmt>(d);
}


// -------------------------------------------------------------------------- //
// Initializers

Trivial_init&
Builder::make_trivial_init(Type& t)
{
  return make<Trivial_init>(t);
}


Copy_init&
Builder::make_copy_init(Type& t, Expr& e)
{
  return make<Copy_init>(t, e);
}


Bind_init&
Builder::make_bind_init(Type& t, Expr& e)
{
  return make<Bind_init>(t, e);
}


Direct_init&
Builder::make_direct_init(Type& t, Decl& d, Expr_list const& es)
{
  return make<Direct_init>(t, d, es);
}


Aggregate_init&
Builder::make_aggregate_init(Type& t, Expr_list const& es)
{
  return make<Aggregate_init>(t, es);
}


// -------------------------------------------------------------------------- //
// Declarations

Super_decl&
Builder::make_super_declaration(Type& t)
{
  Def& d = make_empty_definition();
  return make<Super_decl>(get_id(), t, d);
}

Variable_decl&
Builder::make_variable_declaration(Name& n, Type& t)
{
  Def& d = make_empty_definition();
  return make<Variable_decl>(n, t, d);
}


Variable_decl&
Builder::make_variable_declaration(Name& n, Type& t, Expr& e)
{
  Def& d = make_expression_definition(e);
  return make<Variable_decl>(n, t, d);
}


Variable_decl&
Builder::make_variable_declaration(char const* s, Type& t, Expr& i)
{
  return make_variable_declaration(get_id(s), t, i);
}


// Create a new function. The type is synthesized from the parameter
// and return types, and the definition is synthesized from the given
// expression.
Function_decl&
Builder::make_function_declaration(Name& n, Decl_list const& p, Type& t, Expr& e)
{
  Type& r = get_function_type(p, t);
  Def& d = make_expression_definition(e);
  return make<Function_decl>(n, r, p, d);
}


// Create a new function. The type is synthesized from the parameter
// and return types, and the definition is synthesized from the given
// statement.
Function_decl&
Builder::make_function_declaration(Name& n, Decl_list const& p, Type& t, Stmt& s)
{
  Type& r = get_function_type(p, t);
  Def& d = make_function_definition(s);
  return make<Function_decl>(n, r, p, d);
}


Type_decl&
Builder::make_type_declaration(Name& n, Type& t, Stmt& s)
{
  Def& d = make_type_definition(s);
  return make<Type_decl>(n, t, d);
}


Field_decl&
Builder::make_field_declaration(Name& n, Type& t)
{
  Def& d = make_empty_definition();
  return make<Field_decl>(n, t, d);
}


Field_decl&
Builder::make_field_declaration(Name& n, Type& t, Expr& e)
{
  Def& d = make_expression_definition(e);
  return make<Field_decl>(n, t, d);
}


Method_decl&
Builder::make_method_declaration(Name& n, Decl_list const& p, Type& t, Expr& e)
{
  Type& r = get_function_type(p, t);
  Def& d = make_expression_definition(e);
  return make<Method_decl>(n, r, p, d);
}


Method_decl&
Builder::make_method_declaration(Name& n, Decl_list const& p, Type& t, Stmt& s)
{
  Type& r = get_function_type(p, t);
  Def& d = make_function_definition(s);
  return make<Method_decl>(n, r, p, d);
}



Template_decl&
Builder::make_template(Decl_list const& p, Decl& d)
{
  return make<Template_decl>(p, d);
}


Concept_decl&
Builder::make_concept(Name& n, Decl_list const& ps)
{
  return make<Concept_decl>(n, ps);
}


Concept_decl&
Builder::make_concept(Name& n, Decl_list const& ps, Def& d)
{
  return make<Concept_decl>(n, ps, d);
}


Concept_decl&
Builder::make_concept(Name& n, Decl_list const& ps, Expr& e)
{
  return make<Concept_decl>(n, ps, make_expression_definition(e));
}


Concept_decl&
Builder::make_concept(char const* s, Decl_list const& ps, Def& d)
{
  return make_concept(get_id(s), ps, d);
}


Concept_decl&
Builder::make_concept(char const* s, Decl_list const& ps, Expr& e)
{
  return make_concept(get_id(s), ps, make_expression_definition(e));
}


// TODO: Parameters can't be functions or void. Check this
// property or assert it.
Object_parm&
Builder::make_object_parm(Name& n, Type& t)
{
  return make<Object_parm>(n, t);
}


Object_parm&
Builder::make_object_parm(char const* s, Type& t)
{
  return make_object_parm(get_id(s), t);
}


Value_parm&
Builder::make_value_parm(Name& n, Type& t)
{
  return make<Value_parm>(Index {}, n, t);
}


Value_parm&
Builder::make_value_parm(char const* s, Type& t)
{
  return make_value_parm(get_id(s), t);
}


Type_parm&
Builder::make_type_parameter(Name& n)
{
  return make<Type_parm>(Index {}, n);
}


Type_parm&
Builder::make_type_parameter(char const* n)
{
  return make_type_parameter(get_id(n));
}


// Make a type parameter with a default type.
Type_parm&
Builder::make_type_parameter(Name& n, Type& t)
{
  return make<Type_parm>(Index {}, n, t);
}


// Make a type parameter with a default type.
Type_parm&
Builder::make_type_parameter(char const* n, Type& t)
{
  return make_type_parameter(get_id(n), t);
}


// Create a new placeholder type. This creates a new, uniqe type
// parameter and returns its associated type.
Typename_type&
Builder::make_placeholder_type()
{
  Name& n = get_id();
  Decl& d = make<Type_parm>(Index {}, n);
  return get_typename_type(d);
}




// -------------------------------------------------------------------------- //
// Definitions

Empty_def&
Builder::make_empty_definition()
{
  static Empty_def d;
  return d;
}

Deleted_def&
Builder::make_deleted_definition()
{
  static Deleted_def d;
  return d;
}


Defaulted_def&
Builder::make_defaulted_definition()
{
  static Defaulted_def d;
  return d;
}


Expression_def&
Builder::make_expression_definition(Expr& e)
{
  return make<Expression_def>(e);
}


Function_def&
Builder::make_function_definition(Stmt& s)
{
  return make<Function_def>(s);
}


Type_def&
Builder::make_type_definition(Stmt& s)
{
  return make<Type_def>(s);
}


Concept_def&
Builder::make_concept_definition(Req_list const& ss)
{
  return make<Concept_def>(ss);
}



// -------------------------------------------------------------------------- //
// Requirements

Basic_req&
Builder::make_basic_requirement(Expr& e, Type& t)
{
  return make<Basic_req>(e, t);
}


Conversion_req&
Builder::make_conversion_requirement(Expr& e, Type& t)
{
  return make<Conversion_req>(e, t);
}


Syntactic_req&
Builder::make_syntactic_requirement(Expr& e)
{
  return make<Syntactic_req>(e);
}


// -------------------------------------------------------------------------- //
// Constraints

// FIXME: Save all uniqued terms in the context, not as global variables.

Concept_cons&
Builder::get_concept_constraint(Decl& d, Term_list const& ts)
{
  static Factory<Concept_cons> f;
  return f.make(d, ts);
}


Predicate_cons&
Builder::get_predicate_constraint(Expr& e)
{
  static Factory<Predicate_cons> f;
  return f.make(e);
}


Expression_cons&
Builder::get_expression_constraint(Expr& e, Type& t)
{
  static Factory<Expression_cons> f;
  return f.make(e, t);
}


Conversion_cons&
Builder::get_conversion_constraint(Expr& e, Type& t)
{
  static Factory<Conversion_cons> f;
  return f.make(e, t);

}


Parameterized_cons&
Builder::get_parameterized_constraint(Decl_list const& ds, Cons& c)
{
  static Factory<Parameterized_cons> f;
  return f.make(ds, c);
}


Conjunction_cons&
Builder::get_conjunction_constraint(Cons& c1, Cons& c2)
{
  static Factory<Conjunction_cons> f;
  return f.make(c1, c2);
}


Disjunction_cons&
Builder::get_disjunction_constraint(Cons& c1, Cons& c2)
{
  static Factory<Disjunction_cons> f;
  return f.make(c1, c2);
}


}
