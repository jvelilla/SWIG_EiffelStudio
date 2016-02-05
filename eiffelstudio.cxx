/**
 * \file eiffelstudio.cxx
 * \author  Louis Marchand <prog@tioui.com>
 * \version 1.0
 *
 * \section LICENSE
 *
 * The MIT License (MIT)
 * 
 * Copyright (c) 2016 Louis Marchand
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * \section DESCRIPTION
 *
 * The EIFFELSTUDIO class redefine SWIG Language functionnality for
 * EiffelStudio Wrapper
 */

#include <iostream>
#include "swigmod.h"

class EIFFELSTUDIO : public Language {
private:

	void add_return_type(Hash *a_methods_values, SwigType *a_type);
	void add_arguments(ParmList *a_values, ParmList *a_parms);
	Hash * get_type_value(SwigType *a_type);
	bool managing_class;
	String * externalType();
	void inlineWrapper(Node *n, Hash * a_signature_values, String * a_alias);

protected:
	File *f_module;
	File *f_not_used;
	Hash *h_type_map;
	String *class_cast;
public:

	virtual void main(int argc, char *argv[]);

	virtual int top(Node *n);

	virtual int functionWrapper(Node *n);

	virtual int memberfunctionHandler(Node *n);

	virtual int constantWrapper(Node *n);

	virtual int membervariableHandler(Node *n);
   
	virtual int structConstructorHandler(Node *n);

	virtual int classConstructorHandler(Node *n);

	virtual int constructorHandler(Node *n);
   
	virtual int destructorHandler(Node *n);

	virtual int globalvariableHandler(Node *n);

	virtual int classDeclaration(Node *n);

	virtual void manage_overloading(Node *n);
};
extern "C" Language *
swig_eiffelstudio(void) {
  return new EIFFELSTUDIO();
}

/**
 * \brief Map C type to there associated Eiffel Type
 *
 * Map each C type to it's associated Eiffel type. Every C pointer are
 * repersent byt the key '*' and are pat to the Eiffel type 'POINTER'
 *
 * \return Hash map where the Key is a C type and the value is the Eiffel type
 */
Hash * create_type_map(){
	Hash * map = NewHash();
	Setattr(map, "*", "POINTER");
	Setattr(map, "int", "INTEGER");
	Setattr(map, "signed", "INTEGER");
	Setattr(map, "signed int", "INTEGER");
	Setattr(map, "unsigned int", "NATURAL");
	Setattr(map, "unsigned", "NATURAL");
	Setattr(map, "char", "INTEGER_8");
	Setattr(map, "signed char", "INTEGER_8");
	Setattr(map, "unsigned char", "NATURAL_8");
	Setattr(map, "short", "INTEGER_16");
	Setattr(map, "signed short", "INTEGER_16");
	Setattr(map, "unsigned short", "NATURAL_16");
	Setattr(map, "long", "INTEGER_32");
	Setattr(map, "signed long", "INTEGER_32");
	Setattr(map, "unsigned long", "NATURAL_32");
	Setattr(map, "long long", "INTEGER_64");
	Setattr(map, "signed long long", "INTEGER_64");
	Setattr(map, "unsigned long long", "NATURAL_64");
	Setattr(map, "bool", "BOOLEAN");
	Setattr(map, "float", "REAL_32");
	Setattr(map, "double", "REAL_64");
	return map;
}

/**
 * The Main function. Launched by the program Main.
 *
 * \param argc The number of element of `argv'
 * \param argv Every parameters used in the 'swig' command line
 */
void EIFFELSTUDIO::main(int argc, char *argv[]) {
	SWIG_library_directory("eiffelstudio");
	Preprocessor_define("EIFFELSTUDIO 1", 0);
	SWIG_config_file("eiffelstudio.swg");
	SWIG_typemap_lang("eiffelstudio");
}

/**
 * \brief Parsing the root of the syntax tree
 *
 * When the parser start, the top node (root of the parsing tree)
 * is managed by this function.
 *
 * \param n The root of the SWIG parsing tree
 *
 * \return SWIG_OK if no error and SWIG_ERROR on error
 */
int EIFFELSTUDIO::top(Node *n) {
	String *module = Swig_string_lower(Getattr(n,"name"));
	String *filen = NewStringf("%s%s.e", SWIG_output_directory(), module);
	f_module = NewFile(filen, "w", SWIG_output_files());
	if (!f_module) {
		FileErrorDisplay(module);
		SWIG_exit(EXIT_FAILURE);
	}
	class_cast = NewString("");
	f_not_used = NewString(""); // Usefull for storing the unused SWIG run-time
	h_type_map = create_type_map();
	Swig_register_filebyname("header", f_module);
	Swig_register_filebyname("wrapper", f_module);
 	Swig_register_filebyname("runtime", f_not_used);
	Language::top(n);
	Delete(class_cast);
	Delete(filen);
	Delete(module);
	Delete(f_module);
	Delete(f_not_used);
	Delete(h_type_map);
	return SWIG_OK;
}

/**
 * \brief Extract the C type and Eiffel Type from a Swig type
 *
 * The resulting Hash map contain two key. The first one is `Ctype' and
 * the value is the C type associated with the SWIG type. The second is
 * `Etype' and is the Eiffel Type associated with the SWIG type.
 *
 * \param a_type A SWIG type
 *
 * \return Hash map containing the C and Eiffel type (Callers must `Delete' it)
 */
Hash *EIFFELSTUDIO::get_type_value(SwigType *a_type){
	String *Ctypestr = NULL;
	String *Etypestr = NULL;
	String * tempstr;
   	String * tempstr2;
	SwigType *resolve_type = SwigType_typedef_resolve_all(a_type);
	Hash *result = NewHash();
	if(SwigType_ispointer(resolve_type) or SwigType_isarray(resolve_type)){
	   	Ctypestr = NewString(SwigType_str(SwigType_ltype(a_type), NULL));
		Etypestr = NewString(Getattr(h_type_map,"*"));
	} else if(SwigType_issimple(a_type)) {
		Ctypestr = NewString(SwigType_str(a_type, NULL));
		if (Strcmp(Ctypestr, "void")) {
			tempstr2 = SwigType_str(resolve_type, NULL);
			tempstr = Getattr(h_type_map,tempstr2);
			Delete(tempstr2);
			if (tempstr) {
				Etypestr = NewString(tempstr);
			} else {
				Etypestr = NewString("UNKNOWN");
			}
		} else {
			Delete(Ctypestr);
			Ctypestr = NULL;
		}
	}
	if(Ctypestr and Etypestr){
		Replace(Ctypestr, " const", "", DOH_REPLACE_ANY);
		Replace(Ctypestr, "const ", "", DOH_REPLACE_ANY);
		Replace(Ctypestr, "const", "", DOH_REPLACE_ANY);
		Setattr(result, "Ctype", Ctypestr);
		Setattr(result, "Etype", Etypestr);
		Delete(Ctypestr);
		Delete(Etypestr);
	}
	Delete(resolve_type);
	return result;
}

/**
 * \brief Add the C and Eiffel return type to the methods values Hash map
 *
 * Get the C and Eiffel type from the SWIG type `a_type' and add two keys to
 * the `a_methods_values' object. The first one is `Creturn' and
 * the value is the C type associated with the SWIG type. The second is
 * `Ereturn' and is the Eiffel Type associated with the SWIG type.
 *
 * \param a_methods_values The Hash that the return types have to be added
 * \param a_type The SWIG type representing a C method return type
 * \note This function has a side effect on the argument `a_methods_values'
 */
void EIFFELSTUDIO::add_return_type(Hash *a_methods_values, SwigType *a_type){
	Hash * l_types = get_type_value(a_type);
	String * tempstr;
	if (Getattr(l_types, "Ctype") && Getattr(l_types, "Etype")) {
		tempstr = NewStringf(": %s", Getattr(l_types, "Ctype"));
		Setattr(a_methods_values, "Creturn", tempstr);
		Delete(tempstr);
		tempstr = NewStringf(": %s", Getattr(l_types, "Etype"));
		Setattr(a_methods_values, "Ereturn", tempstr);
		Delete(tempstr);
	} else {
		tempstr = NewString("");
		Setattr(a_methods_values, "Creturn", tempstr);
		Setattr(a_methods_values, "Ereturn", tempstr);
		Delete(tempstr);
	}
	Delete(l_types);
}

/**
 * \brief Add the C and Eiffel arguments types to the arguments values string
 *
 * Get the C and Eiffel types from every arguments in `a_parms' and add
 * two String to the Hash map `a_values'. The first string is the C
 * arguments types string and is identified by the key 'Cparm'.
 * The second string is the Eiffel arguments string and is identified by the
 * key 'Eparm'.
 *
 * The 'Cparm' string have the form:
 * (Ctype1, Ctype2, ...)
 *
 * The 'Eparm' string have the form:
 * (a_name1:Etype1; a_name2:Etype2; ...)
 *
 * \param a_values The Hash that the Strings have to be added
 * \param a_parms A list of every argument of the methods that is parsing.
 * \note This function has a side effect on the argument `a_values'
 */
void EIFFELSTUDIO::add_arguments(ParmList *a_values, ParmList *a_parms) {
	SwigType *type;
	Hash * l_types = NULL;
	String *name;
	String *eArguments = NULL;
	String *eCparams = NULL;
	String *eCcalls = NULL;
	String * tempstr;
	Parm *p;
	int i = 0;
	for(p = a_parms; p; p = nextSibling(p)){
		i = i + 1;
		if(p){
			type  = Getattr(p,"type");
			tempstr = NewString(Getattr(p,"name"));
			name = Swig_string_lower(tempstr);
			Delete(tempstr);
			if(Len(name) == 0){
				Delete(name);
				name = NewStringf("argument%d", i);
			}
			l_types = get_type_value(type);
			if (!eArguments) {
				eArguments = NewStringf("(a_%s: %s", name, Getattr(l_types, "Etype"));
				eCparams = NewStringf(" (%s", Getattr(l_types, "Ctype"));
				eCcalls = NewStringf(" ((%s)$a_%s", Getattr(l_types, "Ctype"), name);
			} else {
				tempstr = NewStringf("; a_%s: %s", name, Getattr(l_types, "Etype"));
				Append(eArguments, tempstr);
				Delete(tempstr);
				tempstr = NewStringf(", %s", Getattr(l_types, "Ctype"));
				Append(eCparams, tempstr);
				Delete(tempstr);
				tempstr = NewStringf(", (%s)$a_%s", Getattr(l_types, "Ctype"), name);
				Append(eCcalls, tempstr);
				Delete(tempstr);
			}
			Delete(name);
			Delete(l_types);
			l_types = NULL;
		}
	}
	if (eArguments && eCparams && eCcalls) {
		Append(eArguments, ")");
		Append(eCparams, ")");
		Append(eCcalls, ")");
		Setattr(a_values, "Cparm", eCparams);
		Setattr(a_values, "Eparm", eArguments);
		Setattr(a_values, "Ccalls", eCcalls);
		Delete(eArguments);
		Delete(eCparams);
		Delete(eCcalls);
	} else {
		tempstr = NewString("");
		Setattr(a_values, "Cparm", tempstr);
		Setattr(a_values, "Eparm", tempstr);
		Delete(tempstr);
		tempstr = NewString("()");
		Setattr(a_values, "Ccalls", tempstr);
		Delete(tempstr);
	}
}

/**
 * \brief Return the string indicating the external type (C or C++)
 *
 * \return The String "C" if in C mode, "C++" if in C++ mode
 */
String * EIFFELSTUDIO::externalType(){
	String * result = NULL;
	if (CPlusPlus){
		result = NewString("C++");
	} else {
		result = NewString("C");
	}
	return result;
}

void EIFFELSTUDIO::inlineWrapper(
		Node *n, Hash * a_signature_values, String * a_alias) {
	String *header = Getattr(n,"feature:h_file");
	String *prefix = Getattr(n,"feature:prefix");
	String *sufix = Getattr(n,"feature:sufix");
	String *external_type = externalType();
	String *Cheader;
	if (header) {
		Cheader = NewStringf(" use %s", header);
	} else {
		Cheader = NewString("");
	}
	Printf(f_module, "\tfrozen %s%s%s%s%s\n",
			prefix, Getattr(a_signature_values, "name"), sufix,
			Getattr(a_signature_values, "Eparm"), 
			Getattr(a_signature_values, "Ereturn"));
	Printf(f_module, "\t\texternal\n");
	Printf(f_module, "\t\t\t\"%s inline%s\"\n", external_type, Cheader);
	Printf(f_module, "\t\talias\n");
	Printf(f_module, "\t\t\t\"%s\"\n", a_alias);
	Printf(f_module, "\t\tend\n\n");
	Delete(external_type);
	Delete(Cheader);
}

/**
 * \brief Manage a function tree node
 *
 * Generate a function Eiffel Wrapper from the function parsing
 * node `n'.
 *
 * \param n The function parsing node
 *
 * \return SWIG_OK if no error and SWIG_ERROR on error
 */
int EIFFELSTUDIO::functionWrapper(Node *n) {
	String   *name   = Getattr(n,"sym:name");
	SwigType *type   = Getattr(n,"type");
	ParmList *parms  = Getattr(n,"parms");
	Hash *l_values = NewHash();
	String *l_alias;
	Setattr(l_values, "name", name);
	add_return_type(l_values, type);
	add_arguments(l_values, parms);
	if (Len(Getattr(l_values, "Ereturn")) > 0) {
		l_alias = NewStringf("return %s%s", name, Getattr(l_values, "Ccalls"));
	} else {
		l_alias = NewStringf("%s%s", name, Getattr(l_values, "Ccalls"));
	}
	inlineWrapper(n, l_values, l_alias);
	Delete(l_alias);
	Delete(l_values);
	return SWIG_OK;
}

/**
 * \brief Manage a function member of a class tree node
 *
 * Generate a class function Eiffel Wrapper from the class function parsing
 * node `n'.
 *
 * \param n The function member class parsing node
 *
 * \return SWIG_OK if no error and SWIG_ERROR on error
 */
int EIFFELSTUDIO::memberfunctionHandler(Node *n) {
	manage_overloading(n);
	String *name   = Getattr(n,"sym:name");
	SwigType *type   = Getattr(n,"type");
	ParmList *parms  = Getattr(n,"parms");
	String *class_prefix = getClassPrefix();
	String *fname = Swig_name_member(getNSpace(), class_prefix, name);
	Hash *l_values = NewHash();
	String *l_alias;
	int replace_count = 0;
	add_return_type(l_values, type);
	add_arguments(l_values, parms);
	replace_count = Replace(Getattr(l_values, "Eparm"), "(", "(a_self:POINTER; ", 
			DOH_REPLACE_FIRST);
	if(not replace_count){
		Setattr(l_values, "Eparm", "(a_self:POINTER)");
	}
	Setattr(l_values, "name", fname);
	if (Len(Getattr(l_values, "Ereturn")) > 0) {
		l_alias = NewStringf("return ((%s *)$a_self)->%s%s", class_cast, name,
			Getattr(l_values, "Ccalls"));
	} else {
		l_alias = NewStringf("((%s *)$a_self)->%s%s", class_cast, name,
			Getattr(l_values, "Ccalls"));
	}
	inlineWrapper(n, l_values, l_alias);
	Delete(fname);
	Delete(l_values);
	return SWIG_OK;
}

/**
 * \brief Manage a constant tree node
 *
 * Generate a constant Eiffel Wrapper from the constant parsing
 * node `n'.
 *
 * \param n The constant parsing node
 *
 * \return SWIG_OK if no error and SWIG_ERROR on error
 */
int EIFFELSTUDIO::constantWrapper(Node *n) {
	manage_overloading(n);
	String   *name   = Getattr(n,"sym:name");
	SwigType *type   = Getattr(n,"type");
	String *numeric_type = Getattr(n,"feature:numeric_define_type");
	String *stringType = SwigType_str(type, NULL);
	String *external_type = externalType();   
	String *l_alias;
	Hash *l_values = get_type_value(type);
	if ((numeric_type) && (!Strcmp(stringType, "int"))) {
		type = numeric_type;
	}
	int result = SWIG_OK;
	if (Getattr(l_values, "Etype")) {
		Setattr(l_values, "name", name);
		l_alias = NewStringf("return %s", name);
		inlineWrapper(n, l_values, l_alias);
	}  else {
        Printf(stdout, "Cannot get Eiffel Type from C type %s.", stringType);
        result = SWIG_ERROR;
    }
	Delete(stringType);
	Delete(external_type);
	Delete(l_values);
	return result;
}

/**
 * \brief Manage a structure member tree node
 *
 * Generate a structure member Eiffel Wrapper from the structure member
 * parsing node `n'. The generated Wrapper will have a getter and a setter for
 * the struct member.
 *
 * \param n The structure member parsing node
 *
 * \return SWIG_OK if no error and SWIG_ERROR on error
 */
int EIFFELSTUDIO::membervariableHandler(Node *n){
	manage_overloading(n);
	String *name   = Getattr(n,"sym:name");
	SwigType *type   = Getattr(n,"type");
	String *class_prefix = getClassPrefix();
	String *mrename_get;
	String *mrename_set;
	String *mname;
	String *l_alias;
	String *tempstr;
	String *stringType = SwigType_str(type, NULL);
	Hash *l_types = get_type_value(type);
	Hash * l_values = NewHash();
	mname = Swig_name_member(0, class_prefix, name);
	mrename_get = Swig_name_get(getNSpace(), mname);
			mrename_set = Swig_name_set(getNSpace(), mname);
	int result = SWIG_OK;
	if (Getattr(l_types, "Etype")) {
		l_alias = NewStringf("return ((%s *)a_self)->%s", class_cast, name);
		Setattr(l_values, "name", mrename_get);
		tempstr = NewStringf(": %s", Getattr(l_types, "Etype"));
		Setattr(l_values, "Ereturn", tempstr);
		Delete(tempstr);
		inlineWrapper(n, l_values, l_alias);	
		if (is_assignable(n)) {
			Delete(l_alias);
			l_alias = NewStringf("((%s *)a_self)->%s = (%s)$a_value", class_cast,
					name, Getattr(l_types, "Ctype"));
			Setattr(l_values, "name", mrename_set);
			tempstr = NewStringf("(a_self:POINTER; a_value:%s)", Getattr(l_types, "Etype"));
			Setattr(l_values, "Eparm", tempstr);
			Setattr(l_values, "Ereturn", NULL);
			Delete(tempstr);
			inlineWrapper(n, l_values, l_alias);	
		}
	} else {
		Printf(stdout, "Cannot get Eiffel Type from C type %s.", stringType);
		result = SWIG_ERROR;
	}
	Delete(l_alias);
	Delete(stringType);
	Delete(mname);
	Delete(mrename_get);
	Delete(mrename_set);
	Delete(l_types);
	return result;
}

/**
 * \brief Manage a structure construction tree node
 *
 * Generate a structure construction Eiffel Wrapper from the structure
 * construction parsing node `n'. The generated Wrapper will have a
 * constructor Eiffel routine (called new_...) and a sizeof routine
 * called (..._size).
 *
 * \param n The structure construction parsing node
 *
 * \return SWIG_OK if no error and SWIG_ERROR on error
 */
int EIFFELSTUDIO::structConstructorHandler(Node *n) {
	String *name   = Getattr(n,"sym:name");
	String *mrename = Swig_name_construct(getNSpace(), name);
	String *size_name;
	Hash *l_values = NewHash();
	String *l_alias;
	size_name = NewStringf("%s_size", name);
	Setattr(l_values, "name", size_name);
	Setattr(l_values, "Ereturn", ":INTEGER");
	l_alias = NewStringf("return sizeof(%s)", class_cast);
	inlineWrapper(n, l_values, l_alias);
	Delete(l_alias);
	Setattr(l_values, "name", mrename);
	Setattr(l_values, "Ereturn", ":POINTER");
	l_alias = NewStringf("return malloc(sizeof(%s))", class_cast);
	inlineWrapper(n, l_values, l_alias);
	Delete(l_alias);
	Delete(l_values);
	Delete(size_name);
	Delete(mrename);
	return SWIG_OK;
}

/**
 * \brief Manage a class construction tree node
 *
 * Generate a class construction Eiffel Wrapper from the class
 * construction parsing node `n'.
 *
 * \param n The class construction parsing node
 *
 * \return SWIG_OK if no error and SWIG_ERROR on error
 */
int EIFFELSTUDIO::classConstructorHandler(Node *n) {
	manage_overloading(n);
	String *name   = Getattr(n,"sym:name");
	ParmList *parms  = Getattr(n,"parms");
	Hash *l_values = NewHash();
	add_arguments(l_values, parms);
	String *mrename = Swig_name_construct(getNSpace(), name);
	String *l_alias;
	Setattr(l_values, "Ereturn", ":POINTER");
	Setattr(l_values, "name", mrename);
	l_alias = NewStringf("return new %s%s", name, Getattr(l_values, "Ccalls"));
	inlineWrapper(n, l_values, l_alias);
	Delete(l_values);
	Delete(l_alias);
	Delete(mrename);
	return SWIG_OK;
}

/**
 * \brief Manage a class/struct/union construction tree node
 *
 * Generate a class/struct/union construction Eiffel Wrapper from the
 * class/struct/union construction parsing node `n'.
 *
 * \param n The class/struct/union construction parsing node
 *
 * \return SWIG_OK if no error and SWIG_ERROR on error
 */
int EIFFELSTUDIO::constructorHandler(Node *n) {
	int result = SWIG_OK;
	manage_overloading(n);
	if(managing_class){
		result = classConstructorHandler(n);
	} else {
		result = structConstructorHandler(n);
	}
	return result;
}

/**
 * \brief Manage a structure destruction tree node
 *
 * Generate a structure destruction Eiffel Wrapper from the structure
 * destruction parsing node `n'. The generated Wrapper will have a
 * destruction Eiffel routine (called delete_...).
 *
 * \param n The structure destruction parsing node
 *
 * \return SWIG_OK if no error and SWIG_ERROR on error
 */
int EIFFELSTUDIO::destructorHandler(Node *n) {
	manage_overloading(n);
	String *name   = Getattr(n,"sym:name");
	String *mrename;
	String *l_values = NewHash();
	String *l_alias;
	char *cname = Char(name);
	if (*cname == '~') {
		cname += 1;
	}
	mrename = Swig_name_destroy(getNSpace(), cname);
	Setattr(l_values, "name", mrename);
	Setattr(l_values, "Eparm", "(a_self:POINTER)");
	if(managing_class){
		l_alias = NewStringf("delete (%s *)$a_self", class_cast);
	} else {
		l_alias = NewStringf("free((%s *)$a_self)", class_cast);
	}
	inlineWrapper(n, l_values, l_alias);
	Delete(l_values);
	Delete(l_alias);
	Delete(mrename);
	return SWIG_OK;
}

/**
 * \brief Manage a global variable tree node
 *
 * Generate a global variable Eiffel Wrapper from the global variable
 * parsing node `n'. The generated Wrapper will have a setter and a getter
 * for the global variable.
 *
 * \param n The global variable parsing node
 *
 * \return SWIG_OK if no error and SWIG_ERROR on error
 */
int EIFFELSTUDIO::globalvariableHandler(Node *n) {
	String *name   = Getattr(n,"sym:name");
	SwigType *type   = Getattr(n,"type");
	Hash * l_values = NewHash();
	String *tempstr;
	String * l_alias;
	Hash *l_types = get_type_value(type);
	String * stringType = SwigType_str(type, NULL);
	int result = SWIG_OK;
	if (Getattr(l_types, "Etype")) {
		Setattr(l_values, "Ereturn", Getattr(l_types, "Etype"));
		tempstr = NewStringf("%s_get", name);
		Setattr(l_values, "name", tempstr);
		Delete(tempstr);
		l_alias = NewStringf("return %s", name);
		inlineWrapper(n, l_values, l_alias);
		if (is_assignable(n)){
			Delete(l_alias);
			Setattr(l_values, "Ereturn", NULL);
			tempstr = NewStringf("(a_value : %s)", Getattr(l_types, "Etype"));
			Setattr(l_values, "Eparm", tempstr);
			Delete(tempstr);
			tempstr = NewStringf("%s_set", name);
			Setattr(l_values, "name", tempstr);
			Delete(tempstr);
			l_alias = NewStringf("%s = $a_value", name);
			inlineWrapper(n, l_values, l_alias);
		}
	} else {
		Printf(stdout, "Cannot get Eiffel Type from C type %s.", stringType);
		result = SWIG_ERROR;
	}
	Delete(stringType);
	Delete(l_alias);
	Delete(l_values);
	Delete(l_types);
	return result;
}

/**
 * \brief Manage a class/struct/union tree node
 *
 * Generate a class/struct/union Eiffel Wrapper from the class/struct/union
 * parsing node `n'. The generated Wrapper will have a setter and a getter
 * for the global variable.
 *
 * \param n The class/struct/union parsing node
 *
 * \return SWIG_OK if no error and SWIG_ERROR on error
 */
int EIFFELSTUDIO::classDeclaration(Node *n) {
	int result;
	String *kind = Getattr(n, "kind");
	Delete(class_cast);
	if (
			(Strcmp(Getattr(n, "sym:name"), Getattr(n, "name")) == 0) &&
			(Strcmp(kind, "struct") == 0 || Strcmp(kind, "union") == 0)
		) {
		class_cast = NewStringf("%s %s", kind, Getattr(n, "sym:name"));
	} else {
		class_cast = NewString(Getattr(n, "sym:name"));
	}
	managing_class = (Strcmp(kind, "class") == 0);
	result = Language::classDeclaration(n);
	managing_class = false;
	return result;
}

/**
 * \brief Manage the allow_overloading feature flag
 *
 * \param n The current parsing node
 */
void EIFFELSTUDIO::manage_overloading(Node *n) {
	allow_overloading(GetFlag(n,"feature:allow_overloading"));
}
