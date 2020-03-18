/*
 * src/tutorial/pname.c
 *
 ******************************************************************************
  This file contains routines that can be bound to a Postgres backend and
  called by the backend in the process of processing queries.  The calling
  format for these routines is dictated by Postgres architecture.
******************************************************************************/

#include "postgres.h"

#include "fmgr.h"
#include "libpq/pqformat.h"			/* needed for send/recv functions */
#include <ctype.h>
#include <string.h>
#include "access/hash.h"

PG_MODULE_MAGIC;

typedef struct person
{
	int     i;
	char	person_name[1];
}PersonName;

int check_input(char *string);
int person_name_judge_equal(PersonName *x, PersonName *y);



int check_input(char *string){

	int len = strlen(string);
	int comma = 0;
	int number = 0;
	int index = 0;
	int comma_number = 0;
	char *title_a = "Dr";
	char *title_b = "Prof";
	char *title_c = "Mr";
	char *title_d = "Ms";

	// get the amount of comma and number
	// get the index of comma in the string
	for(int i = 0; i < len; i++){
		if(string[i] == ','){
			comma++;
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
	if((strstr(string,title_a)!=NULL)||(strstr(string,title_b)!=NULL)||(strstr(string,title_c) != NULL)||(strstr(string,title_d) != NULL)){      {
		return 0;
	}
	// E.g. Ab,Cd minmum valid length is equal to 5
	if(len < 5){
		return 0;
	}
	return 1;
}
}


/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(pname_in);

Datum
pname_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	PersonName *result;

	int len = strlen(str);
	// check valid person name format
	if(check_input(str) == 0){
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				errmsg("invalid input syntax for type %s: \"%s\"",
						"pname", str)));
	}

	result = (PersonName *) palloc(sizeof(len + 1 +VARHDRSZ));
	SET_VARSIZE(result , len + 1 +VARHDRSZ);
	// assign the value of str to the result
	snprintf(result->person_name,len+1,"%s",str);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(pname_out);

Datum
pname_out(PG_FUNCTION_ARGS)
{
	PersonName    *pName = (PersonName *) PG_GETARG_POINTER(0);
	char	   *result;
	
	result = psprintf("$s", pName->person_name);
	PG_RETURN_CSTRING(result);
}

/*****************************************************************************
 * Binary Input/Output functions
 *
 * These are optional.
 *****************************************************************************/

/*****************************************************************************
PG_FUNCTION_INFO_V1(pname_recv);
Datum
pname_recv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	PersonName    *result;
	char *pName = pq_getmsgstring(buf);
	int len = strlen(pName);
	result = (PersonName *) palloc(VARHDRSZ + len + 1);
	SET_VARSIZE(result,VARHDRSZ + len + 1)
	snprintf(result->person_name,len+1,"%s",pName)
	PG_RETURN_POINTER(result);
}
PG_FUNCTION_INFO_V1(pname_send);
Datum
pname_send(PG_FUNCTION_ARGS)
{
	PersonName    *pName = (PersonName *) PG_GETARG_POINTER(0);
	StringInfoData buf;
	pq_begintypsend(&buf);
	pq_sendstring(&buf, pName->person_name);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}
*****************************************************************************/

/*****************************************************************************
 * New Operators
 *
 * person_name_less_than
 * person_name_less_than_or_equal
 * person_name_equal
 * person_name_greater_than_or_equal
 * person_name_greater_than
 * 
 *****************************************************************************/

// compare two person names
// equal: return 0
// else: return strcmp result
int person_name_compare(PersonName *x, PersonName *y){

	int len_x = strlen(x->person_name);
	int len_y = strlen(y->person_name);

	char *person_name_x = x->person_name;
	char *person_name_y = y->person_name;

	char *family_name_x;
	char *family_name_y;
	char *given_name_x;
	char *given_name_y;

	char *family_name_result;
	char *given_name_result;

	int index_x = 0;
	int index_y = 0;

	// get the index of comma of the person name x
	for(int i = 0; i < len_x; i++){
		if(x->person_name[i] == ','){
			index_x = i;
			break;
		}
	}
	// get the index of comma of the person name y
	for(int j = 0; j < len_y; j++){
		if(y->person_name[j] == ','){
			index_y = j;
			break;
		}
	}


	// get the family name of x and y
	strncpy(family_name_x, person_name_x, index_x);
	strncpy(family_name_y, person_name_y, index_y);

	family_name_result = strcmp(family_name_x,family_name_y)

	// get the given name of x and y
	strncpy(given_name_x, person_name_x + index_x + 1, len_x - index_x - 1);
	strncpy(given_name_y, person_name_y + index_y + 1, len_y - index_y - 1);

	// judge the family name
	if(family_name_result != 0){
		return family_name_result;
	}

	// if there is a space after comma
	if(x->person_name[index_x + 1] == ' '){
		given_name_x++;
	}
	if(y->person_name[index_y + 1] == ' '){
		given_name_y++;
	}

	given_name_result = strcmp(given_name_x,given_name_y);

	// judge the given name
	if(given_name_result != 0){
		return given_name_result;
	}
	return 0;
}

PG_FUNCTION_INFO_V1(person_name_less_than);

Datum
// less than: person name x < person name y
person_name_less_than(PG_FUNCTION_ARGS)
{ 
	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *y = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(person_name_compare(x, y) < 0);
}

PG_FUNCTION_INFO_V1(peson_name_less_than_or_equal);

Datum
// less than or equal: person name x <= person name y
peson_name_less_than_or_equal(PG_FUNCTION_ARGS)
{
	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *y = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(person_name_compare(x, y) <= 0);
}

PG_FUNCTION_INFO_V1(person_name_equal);

Datum
// equal: person name x == person name y
person_name_equal(PG_FUNCTION_ARGS)
{
	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *y = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(person_name_compare(x, y) == 0);
}

PG_FUNCTION_INFO_V1(person_name_greater_than_or_equal);

Datum
// greater than or equal: person name x >= person name y
person_name_greater_than_or_equal(PG_FUNCTION_ARGS)
{
	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *y = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(person_name_compare(x, y) >= 0);
}

PG_FUNCTION_INFO_V1(person_name_greater_than);

Datum
// greater than: person name x > person name y
person_name_greater_than(PG_FUNCTION_ARGS)
{
	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *y = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(person_name_compare(x, y) > 0);
}

/*****************************************************************************
 * Support Functions
 *
 * person_name_compare
 * 
 *****************************************************************************/

// PG_FUNCTION_INFO_V1(person_name_compare);

// Datum
// // compare: person name x and person name y
// person_name_compare(PG_FUNCTION_ARGS)
// {
// 	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);
// 	PersonName    *y = (PersonName *) PG_GETARG_POINTER(1);

// 	PG_RETURN_INT32(person_name_compare(x, y));
// }

PG_FUNCTION_INFO_V1(family);

Datum
// return the family part of a name
family(PG_FUNCTION_ARGS)
{
	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);
	int len_x = strlen(x->person_name);
	char *person_name_x = x->person_name;
	int index_x = 0;
	char *family_name_x;
	char *result;

	// get the index of comma of the person name x
	for(int i = 0; i < len_x; i++){
		if(x->person_name[i] == ','){
			index_x = i;
			break;
		}
	}



	// get the family name of x
	strncpy(family_name_x, person_name_x, index_x);

	result = psprintf("%s",family_name_x);

	PG_RETURN_CSSTRING(result);
}

PG_FUNCTION_INFO_V1(given);

Datum
// return the given part of a name
given(PG_FUNCTION_ARGS)
{
	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);

	int len_x = strlen(x->person_name);

	char *person_name_x = x->person_name;

	int index_x = 0;

	// get the index of comma of the person name x
	for(int i = 0; i < len_x; i++){
		if(x->person_name[i] == ','){
			index_x = i;
			break;
		}
	}

	char *given_name_x;

	// get the given name of x
	strncpy(given_name_x, person_name_x + index_x + 1, len_x - index_x - 1);

	// if there is a space after comma
	if(x->person_name[index_x + 1] == ' '){
		given_name_x++;
	}

	char *result;

	result = psprintf("%s",given_name_x);

	PG_RETURN_CSSTRING(result);
}

PG_FUNCTION_INFO_V1(show);

Datum
// return the family part of a name
show(PG_FUNCTION_ARGS)
{
	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);

	int len_x = strlen(x->person_name);

	char *person_name_x = x->person_name;

	int index_x = 0;

	// get the index of comma of the person name x
	for(int i = 0; i < len_x; i++){
		if(x->person_name[i] == ','){
			index_x = i;
			break;
		}
	}

	char *family_name_x;
	char *result;

	// get the family name of x
	strncpy(family_name_x, person_name_x, index_x);

	result = psprintf("%s",family_name_x);

	PG_RETURN_CSSTRING(result);
}

PG_FUNCTION_INFO_V1(person_name_hash);

Datum
person_name_hash(PG_FUNCTION_ARGS){

	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);

	int hash_number = 0;

	int len_x = strlen(x->person_name);

	int index_x = 0;

	// get the index of comma of the person name x
	for(int i = 0; i < len_x; i++){
		if(x->person_name[i] == ','){
			index_x = i;
			break;
		}
	}

	//delete space after comma
	if(x->person_name[index_x + 1] == ' '){

		char *tmp_a = x->person_name;
		char *tmp_b = x->person_name;

		for(int j = 0; j < len_x; j++){
			if( j != index_x + 1){
				*tmp_a++ = *tmp_b;
				*tmp_a = '\0';
			}
			tmp_b++;
		}
		
	}

	hash_number = DatumGetUInt32(hash_any((unsigned char *) x->person_name, len_x));
	PG_RETURN_INT32(hash_number); 
}