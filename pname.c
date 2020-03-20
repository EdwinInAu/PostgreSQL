/*
 * src/tutorial/pname.c
 *
 ******************************************************************************
  This file contains routines that can be bound to a Postgres backend and
  called by the backend in the process of processing queries.  The calling
  format for these routines is dictated by Postgres architecture.
******************************************************************************/

#include "postgres.h"
#include <stdio.h>
#include <stdlib.h>
#include "fmgr.h"
#include "libpq/pqformat.h"			/* needed for send/recv functions */
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include "access/hash.h"

PG_MODULE_MAGIC;

typedef struct person
{
	int     length;
	char	person_name[1];
}PersonName;

int check_input(char *string);
int person_name_compare(PersonName *x, PersonName *y);

int check_input(char *string){

	int len = strlen(string);
	int comma_number = 0;
	int index = 0;
	char *title_a = "Dr";
	char *title_b = "Prof";
	char *title_c = "Mr";
	char *title_d = "Ms";

	// get the amount of comma
	// get the index of comma in the string
	for(int i = 0; i < len; i++){
		if(string[i] == ','){
			comma_number++;
			index = i;
		}
		if(string[i] != ' ' && string[i] != ',' && string[i] != '-' && !isalpha(string[i])){
			return 0;
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

	result = (PersonName *) palloc(len + 1 +VARHDRSZ);
	SET_VARSIZE(result , len + 1 +VARHDRSZ);
	// assign the value of str to the result
	snprintf(result->person_name,len+1,"%s",str);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(pname_out);

Datum
pname_out(PG_FUNCTION_ARGS)
{
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

	const char *personName = pq_getmsgstring(buf);
	int len = strlen(personName);

	result = (PersonName *) palloc(VARHDRSZ + len + 1);
	SET_VARSIZE(result,VARHDRSZ + len + 1);

	snprintf(result->person_name,len+1,"%s",personName);

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

	int family_name_result;
	int given_name_result;

	int index_x = 0;
	int index_y = 0;
    char *family_name_x;
    char *family_name_y;
    char *given_name_x;
    char *given_name_y;

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

	family_name_x = (char*) malloc(sizeof(char)*(index_x+2));
    if (family_name_x == NULL){ 
		exit (1);
	}

	family_name_y = (char*) malloc(sizeof(char)*(index_y+2));
    if (family_name_y == NULL){ 
		exit (1);
	}

	// get the family name of x and y
	strncpy(family_name_x, x->person_name, index_x);
	family_name_x[index_x] = '\0';
	strncpy(family_name_y, y->person_name, index_y);
	family_name_y[index_y] = '\0';

    // compare family name x and y
	family_name_result = strcmp(family_name_x,family_name_y);

	// judge the family name
	if(family_name_result != 0){
		return family_name_result;
	}

	// get the given name of x and y
    given_name_x = strchr(x->person_name, ',');
    given_name_y = strchr(y->person_name, ',');

	given_name_x++;
	given_name_y++;

	// if there is a space after comma
	if(x->person_name[index_x + 1] == ' '){
		given_name_x++;
	}
	if(y->person_name[index_y + 1] == ' '){
		given_name_y++;
	}

    // compare the given name
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

PG_FUNCTION_INFO_V1(person_name_less_than_or_equal);

Datum
// less than or equal: person name x <= person name y
person_name_less_than_or_equal(PG_FUNCTION_ARGS)
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
 * family
 * given
 * show
 * 
 *****************************************************************************/

PG_FUNCTION_INFO_V1(family);

Datum
// return the family part of a name
family(PG_FUNCTION_ARGS)
{
	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);

	int len = strlen(x->person_name);
	int index = 0;
    char *family_name;

	// get the index of comma of the person name x
	for(int i = 0; i < len; i++){
		if(x->person_name[i] == ','){
			index = i;
			break;
		}
	}

    family_name = (char*) malloc(sizeof(char)*(index+2));
    if (family_name == NULL){ 
		exit (1);
	}
    
	// get the family name of x
	strncpy(family_name, x->person_name, index);
	family_name[index] = '\0';

	PG_RETURN_CSTRING(family_name);
}

PG_FUNCTION_INFO_V1(given);

Datum
// return the given part of a name
given(PG_FUNCTION_ARGS)
{
	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);

	int len = strlen(x->person_name);

	int index = 0;
    char *given_name;

	// get the index of comma of the person name x
	for(int i = 0; i < len; i++){
		if(x->person_name[i] == ','){
			index = i;
			break;
		}
	}
    // get the given name of x
    given_name = strchr(x->person_name, ',');

	given_name++;

	// if there is a space after comma
	if(x->person_name[index + 1] == ' '){
		given_name++;
	}

	PG_RETURN_CSTRING(given_name);
}

PG_FUNCTION_INFO_V1(show);

Datum
// return the full part of a name
// format: Given(single) Family
show(PG_FUNCTION_ARGS)
{
	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);

	int len_x = strlen(x->person_name);

	int index = 0;

    int space_index = 0;
    int len_given_name;

    char *family_name;
    char *given_name;
    char *tmp;
    char *full_name;

	// get the index of comma of the person name x
	for(int i = 0; i < len_x; i++){
		if(x->person_name[i] == ','){
			index = i;
			break;
		}
	}

    family_name = (char*) malloc(sizeof(char) * (index+2));

    if (family_name == NULL){ 
		exit (1);
	}

	// get the family name of x
	strncpy(family_name, x->person_name, index);

	family_name[index] = '\0';

	// get the given name of x
    given_name = strchr(x->person_name, ',');

	given_name++;

	// if there is a space after comma
	if(x->person_name[index + 1] == ' '){
		given_name++;
	}

    len_given_name = strlen(given_name);

    // use char* tmp to get the single given name  
    // e.g. given_name = Edwin Wang
    // only get the name before space: Edwin
    tmp = (char*) malloc(sizeof(char)*(len_given_name+2));
    if (tmp == NULL){ 
		exit (1);
	}

	for(int j = 0; j < strlen(given_name); j++){
		if(given_name[j] == ' '){
            space_index = j;
			break; 
		}
	}
    if(space_index > 0){
        strncpy(tmp, given_name, space_index);
        tmp[space_index] = '\0';
    }else{
        strcpy(tmp,given_name);
    }

	//+2  1 for the zero-terminator, 1 for the space in the middle
    full_name = (char*) malloc(sizeof(char *)*(strlen(family_name)+strlen(given_name)+2));
    if (full_name == NULL){ 
		exit (1);
	}

	// combine given_name, space and family_name
	// e.g. Given Family
    strcpy(full_name, tmp);
    strcat(full_name, " ");
	strcat(full_name, family_name);
	PG_RETURN_CSTRING(full_name);
}

PG_FUNCTION_INFO_V1(person_name_hash);

Datum
person_name_hash(PG_FUNCTION_ARGS){

	PersonName    *x = (PersonName *) PG_GETARG_POINTER(0);

	int hash_number = 0;

	int len = strlen(x->person_name);

	int index = 0;

    int len_full_time;

    char *family_name;
    char *given_name;
    char *full_name;

    ///delete space after comma

    // get the index of comma of the person name x
	for(int i = 0; i < len; i++){
		if(x->person_name[i] == ','){
			index = i;
			break;
		}
	}

    family_name = (char*) malloc(sizeof(char) * (index+2));
    if (family_name == NULL){ 
		exit (1);
	}

	// get the family name of x
	strncpy(family_name, x->person_name, index+1);

	family_name[index+1] = '\0';

    // get tge given name of x
    given_name = strchr(x->person_name, ',');

	given_name++;

	// if there is a space after comma
	if(x->person_name[index + 1] == ' '){
		given_name++;
	}

    full_name = (char*) malloc(sizeof(char *)*(strlen(family_name)+strlen(given_name)+2));
    if (full_name == NULL){ 
		exit (1);
	}

	// combine family_name, comma and given_name
	// e.g. Family,Given
    strcpy(full_name, family_name);
	strcat(full_name, given_name);

	len_full_time = strlen(full_name) + 1;

	hash_number = DatumGetUInt32(hash_any((unsigned char *) full_name, len_full_time));
	PG_RETURN_INT32(hash_number); 
}
