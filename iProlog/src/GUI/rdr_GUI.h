/** Header file generated with fdesign on Sun May 12 00:55:51 2002.**/

#ifndef FD_rdr_h_
#define FD_rdr_h_

/** Callbacks, globals and object handlers **/
extern void conditions_cb(FL_OBJECT *, long);
extern void make_button_cb(FL_OBJECT *, long);
extern void close_button_cb(FL_OBJECT *, long);
extern void conclusion_cb(FL_OBJECT *, long);


/**** Forms and Objects ****/
typedef struct {
	FL_FORM *rdr;
	void *vdata;
	char *cdata;
	long  ldata;
	FL_OBJECT *cornerstone_case;
	FL_OBJECT *conditions;
	FL_OBJECT *make_button;
	FL_OBJECT *close_button;
	FL_OBJECT *conclusion;
	FL_OBJECT *new_case;
	FL_OBJECT *new_rule;
} FD_rdr;

extern FD_rdr * create_form_rdr(void);

#endif /* FD_rdr_h_ */
