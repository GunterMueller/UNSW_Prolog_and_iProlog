#!/usr/bin/wish
#############################################################################
# Visual Tcl v1.09 Project
#

#################################
# GLOBAL VARIABLES
#
global prolog_proc; 
global widget; 

#################################
# USER DEFINED PROCEDURES
#


proc get_daemon {facet} {
global prolog_proc current_frame current_slot

    .browser.entry_frame.value_text delete 0.0 end

    puts $prolog_proc "list_daemon($current_frame, '$current_slot', $facet)!\nprint('.')!flush_output!\n"
    set input_coming 1
    while {$input_coming} {
        set str [gets $prolog_proc]
        if {$str == "."} {
            set input_coming 0
        } else {
            .browser.entry_frame.value_text insert end "$str\n"
        }
    }
}

proc get_facets {slot} {
global prolog_proc current_frame current_slot

    .browser.browser_frame.browser_frame3.browser3_list delete 0 end
    .browser.entry_frame.value_text delete 0.0 end

    set current_slot $slot
    puts $prolog_proc "list_facets($current_frame, $slot)!\nprint('.')!flush_output!\n"
    set input_coming 1
    while {$input_coming} {
        set str [gets $prolog_proc]
        if {$str == "."} {
            set input_coming 0
        } else {
            .browser.browser_frame.browser_frame3.browser3_list insert end $str
        }
    }
}

proc set_frames {frames} {

    .browser.browser_frame.browser_frame1.browser1_list delete 0 end
    .browser.browser_frame.browser_frame2.browser2_list delete 0 end
    .browser.browser_frame.browser_frame3.browser3_list delete 0 end
    .browser.entry_frame.inherits_frame.inherits_entry delete 0 end
    .browser.entry_frame.value_text delete 0.0 end

    .browser.browser_frame.browser_frame1.browser1_list insert end $frames
}

proc get_inherits {frame} {
global prolog_proc

    .browser.entry_frame.inherits_frame.inherits_entry delete 0 end

    puts $prolog_proc "list_inherits($frame),flush_output!\n"
    set str [gets $prolog_proc]
    .browser.entry_frame.inherits_frame.inherits_entry insert 0 $str
}

proc get_slots {frame} {
global prolog_proc current_frame

    .browser.browser_frame.browser_frame2.browser2_list delete 0 end
    .browser.browser_frame.browser_frame3.browser3_list delete 0 end
    .browser.entry_frame.inherits_frame.inherits_entry delete 0 end
    .browser.entry_frame.value_text delete 0.0 end

    set current_frame $frame
    puts $prolog_proc "list_slots($frame)!\nprint('.')!flush_output!\n"
    set input_coming 1
    while {$input_coming} {
        set str [gets $prolog_proc]
        if {$str == "."} {
            set input_coming 0
        } else {
            .browser.browser_frame.browser_frame2.browser2_list insert end $str
        }
    }

    get_inherits $frame
}

proc open_file {} {
global prolog_proc

    #   Type names		Extension(s)	Mac File Type(s)
    #
    #---------------------------------------------------------
    set types {
	{"Prolog files"		{.pro .pl}	}
	{"Text files"		{.txt .doc}	}
	{"Text files"		{}		TEXT}
	{"All files"		*}
    }

    set file [tk_getOpenFile -filetypes $types -parent .browser  -initialfile "/home/claude/iprolog/examples/cyl.pro"]

    if {$file != ""} {
        puts $prolog_proc "load '$file'!\n"
        get_frames
    }
}

proc main {} {
global prolog_proc

#   set prolog_proc [open "|prolog /home/claude/iprolog/examples/cyl.pro" r+]
#   set prolog_proc [open "|prolog" r+]
#   fconfigure $prolog_proc -buffering line

#   get_frames
}

proc Window {args} {
global vTcl
	set cmd [lindex $args 0]
	set name [lindex $args 1]
	set newname [lindex $args 2]
	set rest [lrange $args 3 end]
    if {$name == "" || $cmd == ""} {return}
	if {$newname == ""} {
		set newname $name
	}
    set exists [winfo exists $newname]
    switch $cmd {
        show {
            if {[info procs vTclWindow(pre)$name] != ""} {
                eval "vTclWindow(pre)$name $newname $rest"
            }
            if {[info procs vTclWindow$name] != ""} {
                eval "vTclWindow$name $newname $rest"
            }
            if {[info procs vTclWindow(post)$name] != ""} {
                eval "vTclWindow(post)$name $newname $rest"
            }
        }
        hide    { if $exists {wm withdraw $newname; return} }
        iconify { if $exists {wm iconify $newname; return} }
        destroy { if $exists {destroy $newname; return} }
    }
}

#################################
# VTCL GENERATED GUI PROCEDURES
#

proc vTclWindow. {base} {
    if {$base == ""} {
        set base .
    }
    ###################
    # CREATING WIDGETS
    ###################
    wm focusmodel $base passive
    wm geometry $base 200x200+0+0
    wm maxsize $base 785 570
    wm minsize $base 1 1
    wm overrideredirect $base 0
    wm resizable $base 1 1
    wm withdraw $base
    wm title $base "vt.tcl"
    ###################
    # SETTING GEOMETRY
    ###################
}

proc vTclWindow.browser {base} {
    if {$base == ""} {
        set base .browser
    }
    if {[winfo exists $base]} {
        wm deiconify $base; return
    }
    ###################
    # CREATING WIDGETS
    ###################
    toplevel $base -class Toplevel \
        -background LightGrey 
    wm focusmodel $base passive
    wm geometry $base 468x380+144+203
    wm maxsize $base 800 600
    wm minsize $base 468 380
    wm overrideredirect $base 0
    wm resizable $base 1 1
    wm deiconify $base
    wm title $base "Frame Browser"
    bind $base <Alt-Key-q> {
        exit
    }
    bind $base <Alt-Key-o> {
        open_file
    }
    frame $base.menu_frame \
        -background LightGrey -borderwidth 1 -height 18 -width 30 
    menubutton $base.menu_frame.file_menu \
        -background LightGrey -menu .browser.menu_frame.file_menu.m -padx 4 \
        -pady 3 -text File 
    menu $base.menu_frame.file_menu.m \
        -background LightGrey -cursor {} -tearoff 0 
    $base.menu_frame.file_menu.m add command \
        -label New -state active 
    $base.menu_frame.file_menu.m add command \
        -accelerator Alt-O -command open_file -label Open 
    $base.menu_frame.file_menu.m add command \
        -label Save 
    $base.menu_frame.file_menu.m add command \
        -label {Save As ...} 
    $base.menu_frame.file_menu.m add command \
        -accelerator Alt-Q -command exit -label Quit 
    menubutton $base.menu_frame.frame_menu \
        -background LightGrey -menu .browser.menu_frame.frame_menu.m -padx 4 \
        -pady 3 -text Frame 
    menu $base.menu_frame.frame_menu.m \
        -background LightGrey -cursor {} -tearoff 0 
    $base.menu_frame.frame_menu.m add command \
        -label {New Frame} -state active 
    $base.menu_frame.frame_menu.m add command \
        -label {Delete Frame} 
    menubutton $base.menu_frame.slot_menu \
        -background LightGrey -menu .browser.menu_frame.slot_menu.m -padx 4 \
        -pady 3 -text Slot 
    menu $base.menu_frame.slot_menu.m \
        -background LightGrey -cursor {} -tearoff 0 
    $base.menu_frame.slot_menu.m add command \
        -label {New Slot} -state active 
    $base.menu_frame.slot_menu.m add command \
        -label {Delete Slot} 
    menubutton $base.menu_frame.facet_menu \
        -background LightGrey -menu .browser.menu_frame.facet_menu.m -padx 4 \
        -pady 3 -text Facet 
    menu $base.menu_frame.facet_menu.m \
        -background LightGrey -cursor {} -tearoff 0 
    $base.menu_frame.facet_menu.m add command \
        -accelerator Alt-U -label Update -state active 
    frame $base.browser_frame \
        -background LightGrey -borderwidth 1 -height 94 -width 30 
    frame $base.browser_frame.browser_frame1 \
        -background LightGrey -borderwidth 1 -height 142 -width 139 
    listbox $base.browser_frame.browser_frame1.browser1_list \
        -background #ffffff -width 18 \
        -yscrollcommand {.browser.browser_frame.browser_frame1.browser1_scroll set} 
    bind $base.browser_frame.browser_frame1.browser1_list <Double-Button-1> {
        get_slots [selection get]
    }
    scrollbar $base.browser_frame.browser_frame1.browser1_scroll \
        -background LightGrey \
        -command {.browser.browser_frame.browser_frame1.browser1_list yview} \
        -orient vert -width 12 
    frame $base.browser_frame.browser_frame2 \
        -background LightGrey -borderwidth 1 -height 30 -width 142 
    listbox $base.browser_frame.browser_frame2.browser2_list \
        -background #ffffff -width 18 \
        -yscrollcommand {.browser.browser_frame.browser_frame2.browser2_scroll set} 
    bind $base.browser_frame.browser_frame2.browser2_list <Double-Button-1> {
        get_facets [selection get]
    }
    scrollbar $base.browser_frame.browser_frame2.browser2_scroll \
        -background LightGrey \
        -command {.browser.browser_frame.browser_frame2.browser2_list yview} \
        -orient vert -width 12 
    frame $base.browser_frame.browser_frame3 \
        -background LightGrey -borderwidth 1 -height 30 -width 147 
    scrollbar $base.browser_frame.browser_frame3.browser3_scroll \
        -background LightGrey \
        -command {.browser.browser_frame.browser_frame3.browser3_list yview} \
        -orient vert -width 12 
    listbox $base.browser_frame.browser_frame3.browser3_list \
        -background #ffffff -width 18 \
        -yscrollcommand {.browser.browser_frame.browser_frame3.browser3_scroll set} 
    bind $base.browser_frame.browser_frame3.browser3_list <Double-Button-1> {
        get_daemon [selection get]
    }
    frame $base.entry_frame \
        -background LightGrey -borderwidth 1 -height 404 -width 30 
    frame $base.entry_frame.inherits_frame \
        -background LightGrey -borderwidth 1 -height 30 -width 30 
    label $base.entry_frame.inherits_frame.inherits_label \
        -background LightGrey -justify left -text {Inherits from:} 
    entry $base.entry_frame.inherits_frame.inherits_entry \
        -background #ffffff -width 45 
    text $base.entry_frame.value_text \
        -background #ffffff -wrap none 
    ###################
    # SETTING GEOMETRY
    ###################
    pack $base.menu_frame \
        -anchor center -expand 0 -fill both -side top 
    pack $base.menu_frame.file_menu \
        -anchor center -expand 0 -fill none -side left 
    pack $base.menu_frame.frame_menu \
        -anchor center -expand 0 -fill none -side left 
    pack $base.menu_frame.slot_menu \
        -anchor center -expand 0 -fill none -side left 
    pack $base.menu_frame.facet_menu \
        -anchor center -expand 0 -fill none -side left 
    pack $base.browser_frame \
        -anchor center -expand 1 -fill x -side top 
    pack $base.browser_frame.browser_frame1 \
        -anchor center -expand 1 -fill both -padx 1 -side left 
    pack $base.browser_frame.browser_frame1.browser1_list \
        -anchor center -expand 1 -fill both -side left 
    pack $base.browser_frame.browser_frame1.browser1_scroll \
        -anchor e -expand 0 -fill y -side left 
    pack $base.browser_frame.browser_frame2 \
        -anchor center -expand 1 -fill both -padx 1 -side left 
    pack $base.browser_frame.browser_frame2.browser2_list \
        -anchor center -expand 1 -fill both -side left 
    pack $base.browser_frame.browser_frame2.browser2_scroll \
        -anchor center -expand 0 -fill y -side right 
    pack $base.browser_frame.browser_frame3 \
        -anchor center -expand 1 -fill both -side left 
    pack $base.browser_frame.browser_frame3.browser3_scroll \
        -anchor e -expand 0 -fill y -side right 
    pack $base.browser_frame.browser_frame3.browser3_list \
        -anchor center -expand 1 -fill both -side right 
    pack $base.entry_frame \
        -anchor center -expand 1 -fill both -side bottom 
    pack $base.entry_frame.inherits_frame \
        -anchor center -expand 0 -fill x -side top 
    pack $base.entry_frame.inherits_frame.inherits_label \
        -anchor w -expand 0 -fill none -side left 
    pack $base.entry_frame.inherits_frame.inherits_entry \
        -anchor e -expand 1 -fill x -pady 1 -side left 
    pack $base.entry_frame.value_text \
        -anchor center -expand 1 -fill both -ipadx 2 -ipady 2 -padx 2 -pady 2 \
        -side top 
}

Window show .
Window show .browser

# main
return
