#include <dlfcn.h>
#include <stdio.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

/*
	We use this XID modifier to flag outputs and crtcs as
	fake
*/
#define XID_SPLIT_MOD 0xf00000

/*
	Generated by ./configure:
*/
#include "config.h"

/*
	The skeleton file is created by ./make_skeleton.py

	It contains wrappers around all Xrandr functions which are not
	explicitly defined in this C file, replacing all references to
	crtcs and outputs which are fake with the real ones.
*/
#include "skeleton.h"

/*
	To avoid memory leaks, we simply use a two big buffers to
	store the fake data
*/
static char fake_output_buffer[sizeof(RRCrtc) * 255];
static char fake_crtc_buffer[sizeof(RRCrtc) * 255];

static void _init() __attribute__((constructor));
static void _init() {
	void *xrandr_lib = dlopen(REAL_XRANDR_LIB, RTLD_LAZY | RTLD_GLOBAL);

	/*
		The following macro is defined by the skeleton header. It initializes
		static variables called _XRRfn with references to the real XRRfn
		functions.
	*/
	FUNCTION_POINTER_INITIALIZATIONS;
}

/*
	Helper functions to generate the fake data
*/
static bool check_if_crtc_is_wrong(Display *dpy, XRRScreenResources *resources, RRCrtc crtc) {
	XRRCrtcInfo *info = _XRRGetCrtcInfo(dpy, resources, crtc & ~XID_SPLIT_MOD);
	bool retval = info->width == SPLIT_SCREEN_WIDTH && info->height == SPLIT_SCREEN_HEIGHT;
	Xfree(info);
	return retval;
}

static bool check_if_output_is_wrong(Display *dpy, XRRScreenResources *resources, RROutput output) {
	XRROutputInfo *info = _XRRGetOutputInfo(dpy, resources, output & ~XID_SPLIT_MOD);
	bool retval = info->crtc != 0 && check_if_crtc_is_wrong(dpy, resources, info->crtc);
	Xfree(info);
	return retval;
}

static RRCrtc *append_fake_crtc(int *count, RRCrtc **crtcs, RRCrtc real_crtc) {
	(*count)++;

	assert(sizeof(RRCrtc) * *count < sizeof(fake_crtc_buffer));
	RRCrtc *new_space = (RRCrtc *)fake_crtc_buffer;
	memcpy(new_space, *crtcs, sizeof(RRCrtc) * (*count - 1));
	*crtcs = new_space;

	assert((real_crtc & XID_SPLIT_MOD) == 0L);
	(*crtcs)[*count - 1] = real_crtc | XID_SPLIT_MOD;
}

static RRCrtc *append_fake_output(int *count, RRCrtc **outputs, RRCrtc real_output) {
	(*count)++;

	assert(sizeof(RRCrtc) * *count < sizeof(fake_output_buffer));
	RRCrtc *new_space = (RRCrtc *)fake_output_buffer;
	memcpy(new_space, *outputs, sizeof(RROutput) * (*count - 1));
	*outputs = new_space;

	assert((real_output & XID_SPLIT_MOD) == 0L);
	(*outputs)[*count - 1] = real_output | XID_SPLIT_MOD;
}

static XRRScreenResources *augment_resources(Display *dpy, Window window, XRRScreenResources *retval) {
	int i;
	for(i=0; i<retval->ncrtc; i++) {
		if(check_if_crtc_is_wrong(dpy, retval, retval->crtcs[i])) {
			/* Add a second crtc (becomes the virtual split screen) */
			append_fake_crtc(&retval->ncrtc, &retval->crtcs, retval->crtcs[i]);
			break;
		}
	}
	for(i=0; i<retval->noutput; i++) {
		if(check_if_output_is_wrong(dpy, retval, retval->outputs[i])) {
			/* Add a second output (becomes the virtual split screen) */
			append_fake_output(&retval->noutput, &retval->outputs, retval->outputs[i]);
			break;
		}
	}

	return retval;
}

/*
	Overridden library functions to add the fake output
*/

XRRScreenResources *XRRGetScreenResources(Display *dpy, Window window) {
	XRRScreenResources *retval = augment_resources(dpy, window, _XRRGetScreenResources(dpy, window));
	return retval;
}

XRRScreenResources *XRRGetScreenResourcesCurrent(Display *dpy, Window window) {
	XRRScreenResources *retval = augment_resources(dpy, window, _XRRGetScreenResourcesCurrent(dpy, window));
	return retval;
}

XRROutputInfo *XRRGetOutputInfo(Display *dpy, XRRScreenResources *resources, RROutput output) {
	XRROutputInfo *retval = _XRRGetOutputInfo(dpy, resources, output & ~XID_SPLIT_MOD);

	if(check_if_output_is_wrong(dpy, resources, output)) {
		retval->mm_width /= 2;
		if(output & XID_SPLIT_MOD) {
			retval->name[retval->nameLen - 1] = '_';
			append_fake_crtc(&retval->ncrtc, &retval->crtcs, retval->crtc);
			retval->crtc = retval->crtc | XID_SPLIT_MOD;
		}
	}

	return retval;
}

XRRCrtcInfo *XRRGetCrtcInfo(Display *dpy, XRRScreenResources *resources, RRCrtc crtc) {
	XRRCrtcInfo *retval = _XRRGetCrtcInfo(dpy, resources, crtc & ~XID_SPLIT_MOD);

	if(check_if_crtc_is_wrong(dpy, resources, crtc)) {
		retval->width /= 2;
		if(crtc & XID_SPLIT_MOD) {
			retval->x += retval->width;
		}
	}

	return retval;
}

int XRRSetCrtcConfig(Display *dpy, XRRScreenResources *resources, RRCrtc crtc, Time timestamp, int x, int y, RRMode mode, Rotation rotation, RROutput *outputs, int noutputs) {
	int i;
	if(crtc & XID_SPLIT_MOD) {
		return 0;
	}
	for(i=0; i<noutputs; i++) {
		if(outputs[i] & XID_SPLIT_MOD) {
			return 0;
		}
	}

	return _XRRSetCrtcConfig(dpy, resources, crtc, timestamp, x, y, mode, rotation, outputs, noutputs);
}
