/*
 * src/tutorial/complex.c
 *
 ******************************************************************************
  This file contains routines that can be bound to a Postgres backend and
  called by the backend in the process of processing queries.  The calling
  format for these routines is dictated by Postgres architecture.
******************************************************************************/

#include "postgres.h"
#include<string.h>
#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include "access/hash.h"

//check
//> < = 
//show

PG_MODULE_MAGIC;

typedef struct person
{
	int length;
	char person_name[1];
}PersonName;

int check_input(char *string);
int person_name_compare(PersonName *a, PersonName *b);
/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(pname_in);

int check_input(char *string){

	int len = strlen(string);
	int comma_number = 0;
	int index = 0;
	char *title_a = "Dr";
	char *title_b = "Prof";
	char *title_c = "Mr";
	char *title_d = "Ms";

	// get the amount of comma and number
	// get the index of comma in the string
	for(int i = 0; i < len; i++){
		if(string[i] == ','){
			comma_number++;
			index = i;
		}
		if(isdigit(string[i])){
			return 0;
		}
		// after a space or "-", there is no upper-case letter
		if(string[i] == ' ' || string[i] == '-'){
			if(!isupper(string[i+1])){
				return 0;
			}
		} 
	} 
	// if there is no comma or more than one comma
	if(comma_number != 1){
		return 0;
	}
	// if there is a space before comma
	if(string[index-1] == ' '){
		return 0;
	}
	// if the family name dose not begin with an upper-case letter
	if(!isupper(string[0])){
		return 0;
	}  
	// if there is more than one space after the comma
	if(string[index+2] == ' '){
		return 0;
	} 
	// if there is a space after the comma 
	// but after space there is no upper-case letter  
	if(string[index+1] == ' ' && isupper(string[index+2]) == 0 ){
		return 0;
	}
	// if there is no space after the comma 
	// but after comma there is no upper-case letter
	if(string[index+1] != ' ' && isupper(string[index+1]) == 0){
		return 0;
	}
	// before the comma, there is only one letter 
	// E.g. A, Bc
	if(index == 1){
		return 0;
	}
	// if the last letter of string is space
	if(string[len-1] == ' '){
		return 0;
	}
	// if the string includes "Dr", "Prof","Mr" or "Ms
	if((strstr(string,title_a)!=NULL)||(strstr(string,title_b)!=NULL)||(strstr(string,title_c) != NULL)||(strstr(string,title_d) != NULL)){
		return 0;
	}
	// E.g. Ab,Cd minmum valid length is equal to 5
	if(len < 5){
		return 0;
	}
	return 1;
}

Datum
pname_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
	PersonName *result;
	int length = strlen(str) + 1;

	if(check_input(str) == 0){
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				errmsg("invalid input syntax for type %s: \"%s\"",
						"pname", str)));
	}

	result = (PersonName *) palloc(VARHDRSZ + length);

	SET_VARSIZE(result, VARHDRSZ + length);
	snprintf(result->person_name, length, "%s", str);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(pname_out);

Datum
pname_out(PG_FUNCTION_ARGS)
{
	// select name from Students;
	PersonName    *personName = (PersonName *) PG_GETARG_POINTER(0);
	char	   *result;

	result = psprintf("%s", personName->person_name);
	PG_RETURN_CSTRING(result);
}

/*****************************************************************************
 * Binary Input/Output functions
 *
 * These are optional.
 *****************************************************************************/

PG_FUNCTION_INFO_V1(pname_recv);

Datum
pname_recv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	PersonName    *result;

	const char *person_name = pq_getmsgstring(buf);
	int length = strlen(person_name) + 1;
	
	result = (PersonName *) palloc(VARHDRSZ + length);
	SET_VARSIZE(result, VARHDRSZ + length);
	snprintf(result->person_name, length, "%s", person_name);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(pname_send);

Datum
pname_send(PG_FUNCTION_ARGS)
{
	PersonName    *personName = (PersonName *) PG_GETARG_POINTER(0);

	StringInfoData buf;
	pq_begintypsend(&buf);

	pq_sendstring(&buf, personName->person_name);

	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/*****************************************************************************
 * Operator class for defining B-tree index
 *
 * It's essential that the comparison operators and support function for a
 * B-tree index opclass always agree on the relative ordering of any two
 * data values.  Experience has shown that it's depressingly easy to write
 * unintentionally inconsistent functions.  One way to reduce the odds of
 * making a mistake is to make all the functions simple wrappers around
 * an internal three-way-comparison function, as we do here.
 *****************************************************************************/

// #define Mag(c)	((c)->x*(c)->x + (c)->y*(c)->y)

// static int
// complex_abs_cmp_internal(Complex * a, Complex * b)
// {
// 	double		amag = Mag(a),
// 				bmag = Mag(b);

// 	if (amag < bmag)
// 		return -1;
// 	if (amag > bmag)
// 		return 1;
// 	return 0;
// }

int person_name_compare(PersonName *a, PersonName *b){

	int a_index = 0, b_index = 0;
	char * a_given_name, b_given_name;
	int result;
	for (a_index=0;a_index<strlen(a->person_name);a_index++){
		if(a->person_name[a_index] == ','){
			break;
		}
	}

	for (b_index=0;b_index<strlen(b->person_name);b_index++){
		if(b->person_name[b_index] == ','){
			break;
		}
	}

	// char * a_given_name = strchr(a->pname, ',');
	// a_index = strlen(a->pname) - strlen(a_given_name);
	// char * b_given_name = strchr(a->pname, ',');
	// b_index = strlen(b->pname) - strlen(b_given_name);


	a->person_name[a_index] = '\0';
	b->person_name[b_index] = '\0';
	result = strcmp(a->person_name, b->person_name);
	a->person_name[a_index] = ',';
	b->person_name[b_index] = ',';
	a_given_name = &(a->person_name[a_index]);
	b_given_name = &(b->person_name[b_index]);

	if(result == 0){
		if (a->person_name[a_index + 1] == ' '){
			a_given_name++;
		}
		if (b->person_name[b_index + 1] == ' '){
			b_given_name++;
		}

		result = strcmp(a_given_name, b_given_name);
	}
	return result;
}


PG_FUNCTION_INFO_V1(person_name_less_than);

Datum
person_name_less_than(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(person_name_compare(a,b) < 0);
}

PG_FUNCTION_INFO_V1(peson_name_less_than_or_equal);

Datum
peson_name_less_than_or_equal(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(person_name_compare(a,b) <= 0);
}


PG_FUNCTION_INFO_V1(person_name_equal);

Datum
person_name_equal(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	// PG_RETURN_BOOL(strcmp(a->pname, b->pname) == 0);
	PG_RETURN_BOOL(person_name_compare(a,b) == 0);
}

PG_FUNCTION_INFO_V1(person_name_greater_than_or_equal);

Datum
person_name_greater_than_or_equal(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	// PG_RETURN_BOOL(strcmp(a->pname, b->pname) == 0);
	PG_RETURN_BOOL(person_name_compare(a,b) >= 0);
}

PG_FUNCTION_INFO_V1(person_name_greater_than);

Datum
person_name_greater_than(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	// PG_RETURN_BOOL(strcmp(a->pname, b->pname) == 0);
	PG_RETURN_BOOL(person_name_compare(a,b) > 0);
}

// PG_FUNCTION_INFO_V1(complex_abs_cmp);

// Datum
// complex_abs_cmp(PG_FUNCTION_ARGS)
// {
// 	Complex    *a = (Complex *) PG_GETARG_POINTER(0);
// 	Complex    *b = (Complex *) PG_GETARG_POINTER(1);

// 	PG_RETURN_INT32(complex_abs_cmp_internal(a, b));
// }

PG_FUNCTION_INFO_V1(family);

Datum
family(PG_FUNCTION_ARGS)
{
	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);

	int index_x = 0;
	int len_x = strlen(x->person_name);
	char *result;

	for(int i = 0; i < len_x; i++){
		if(x->person_name[i] == ','){
			index_x = i;
			break;
		}
	}
	
	x->person_name[index_x] = '\0';
	result = psprintf("%s", x->person_name);
	x->person_name[index_x] = ',';

	PG_RETURN_CSTRING(result);
}

// PG_FUNCTION_INFO_V1(given);

// Datum
// given(PG_FUNCTION_ARGS)
// {
// 	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);
// 	char * given_name;
// 	char * result;
// 	int index_x = 0;
// 	int len_x = strlen(x->person_name);

// 	for(int i = 0; i < len_x; i++){
// 		if(x->person_name[i] == ','){
// 			index_x = i;
// 			break;
// 		}
// 	}

// 	// char * a_given_name = strchr(a->pname, ',') + 1;

// 	if(*(given_name) == ' '){
// 		given_name++;
// 	}
	
// 	result = psprintf("%s", given_name);

// 	PG_RETURN_SCTRING(result);
// }

// PG_FUNCTION_INFO_V1(show);


// Datum
// // return the family part of a name
// show(PG_FUNCTION_ARGS)
// {
// 	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);
	
// 	// char * family_name;
// 	char * family_result;
// 	char * given_name;
// 	char * given_result;
// 	char * full_name;
// 	int index_x = 0;
// 	int len_x = strlen(x->person_name);

// 	for(int i = 0; i < len_x; i++){
// 		if(x->person_name[i] == ','){
// 			index_x = i;
// 			break;
// 		}
// 	}

// 	x->person_name[index_x] = '\0';
// 	family_result = psprintf("%s", x->person_name);
// 	x->person_name[index_x] = ',';

// 	if(*(given_name) == ' '){
// 		given_name++;
// 	}
	
// 	given_result = psprintf("%s", given_name);
// 	strcpy(full_name, family_result);
// 	strcat(full_name, given_result);



// PG_RETURN_CSTRING(full_name);
// }
PG_FUNCTION_INFO_V1(pname_hash);

Datum
pname_hash(PG_FUNCTION_ARGS)
{
	PersonName  *x = (PersonName *) PG_GETARG_POINTER(0);
	// delete space after comma
	int hash_code = 0;
	int index_x = 0;
	int len_x = strlen(x->person_name);
	for(int i = 0; i < len_x; i++){
		if(x->person_name[i] == ','){
			index_x = i;
			break;
		}
	}

	// if (x->person_name[index_x + 1] == ' '){
	// 	x->person_name[index_x + 1] == '';
	// }


	hash_code = DatumGetUInt32(hash_any((unsigned char *)x->person_name, strlen(x->person_name)));

	PG_RETURN_INT32(hash_code);
}