/** Header file generated with fdesign on Fri May 10 19:21:30 2002.**/

#ifndef FD_generic_frame_h_
#define FD_generic_frame_h_

/** Callbacks, globals and object handlers **/
extern void generic_facet_list_cb(FL_OBJECT *, long);
extern void new_generic_slot_button_cb(FL_OBJECT *, long);
extern void daemon_cb(FL_OBJECT *, long);
extern void generic_slot_list_cb(FL_OBJECT *, long);
extern void generic_name_cb(FL_OBJECT *, long);
extern void generic_inherits_cb(FL_OBJECT *, long);
extern void generic_delete_button_cb(FL_OBJECT *, long);
extern void generic_accept_button_cb(FL_OBJECT *, long);
extern void instances_menu_cb(FL_OBJECT *, long);
extern void facet_menu_cb(FL_OBJECT *, long);
extern void generic_remove_slot_button_cb(FL_OBJECT *, long);
extern void generic_remove_facet_button_cb(FL_OBJECT *, long);
extern void generic_frame_list_cb(FL_OBJECT *, long);
extern void new_generic_button_cb(FL_OBJECT *, long);
extern void display_generic_button_cb(FL_OBJECT *, long);
extern void quit_button_cb(FL_OBJECT *, long);
extern void save_button_cb(FL_OBJECT *, long);
extern void open_button_cb(FL_OBJECT *, long);


/**** Forms and Objects ****/
typedef struct {
	FL_FORM *generic_frame;
	void *vdata;
	char *cdata;
	long  ldata;
	FL_OBJECT *generic_facet_list;
	FL_OBJECT *new_generic_slot_button;
	FL_OBJECT *daemon;
	FL_OBJECT *generic_slot_list;
	FL_OBJECT *generic_name;
	FL_OBJECT *generic_inherits;
	FL_OBJECT *generic_delete_button;
	FL_OBJECT *generic_accept_button;
	FL_OBJECT *instances_menu;
	FL_OBJECT *facet_menu;
	FL_OBJECT *generic_remove_slot_button;
	FL_OBJECT *generic_remove_facet_button;
	FL_OBJECT *generic_frame_list;
	FL_OBJECT *new_generic_button;
	FL_OBJECT *dispay_generic_frames;
	FL_OBJECT *quit_button;
	FL_OBJECT *save_button;
	FL_OBJECT *open_button;
} FD_generic_frame;

extern FD_generic_frame * create_form_generic_frame(void);

#endif /* FD_generic_frame_h_ */
