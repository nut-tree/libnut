#include "mouse.h"
#include "microsleep.h"

#include <math.h> /* For floor() */

#if defined(IS_MACOSX)
	#include <ApplicationServices/ApplicationServices.h>
#elif defined(USE_X11)
	#include <X11/Xlib.h>
	#include <X11/extensions/XTest.h>
	#include <stdlib.h>
	#include "xdisplay.h"
#endif

#if !defined(M_SQRT2)
	#define M_SQRT2 1.4142135623730950488016887 /* Fix for MSVC. */
#endif

/* Some convenience macros for converting our enums to the system API types. */
#if defined(IS_MACOSX)

#define MMMouseToCGEventType(down, button) \
	(down ? MMMouseDownToCGEventType(button) : MMMouseUpToCGEventType(button))

#define MMMouseDownToCGEventType(button) \
	((button) == (LEFT_BUTTON) ? kCGEventLeftMouseDown \
	                       : ((button) == RIGHT_BUTTON ? kCGEventRightMouseDown \
	                                                   : kCGEventOtherMouseDown))

#define MMMouseUpToCGEventType(button) \
	((button) == LEFT_BUTTON ? kCGEventLeftMouseUp \
	                         : ((button) == RIGHT_BUTTON ? kCGEventRightMouseUp \
	                                                     : kCGEventOtherMouseUp))

#define MMMouseDragToCGEventType(button) \
	((button) == LEFT_BUTTON ? kCGEventLeftMouseDragged \
	                         : ((button) == RIGHT_BUTTON ? kCGEventRightMouseDragged \
	                                                     : kCGEventOtherMouseDragged))

#elif defined(IS_WINDOWS)

#define MMMouseToMEventF(down, button) \
	(down ? MMMouseDownToMEventF(button) : MMMouseUpToMEventF(button))

#define MMMouseUpToMEventF(button) \
	((button) == LEFT_BUTTON ? MOUSEEVENTF_LEFTUP \
	                         : ((button) == RIGHT_BUTTON ? MOUSEEVENTF_RIGHTUP \
	                                                     : MOUSEEVENTF_MIDDLEUP))

#define MMMouseDownToMEventF(button) \
	((button) == LEFT_BUTTON ? MOUSEEVENTF_LEFTDOWN \
	                         : ((button) == RIGHT_BUTTON ? MOUSEEVENTF_RIGHTDOWN \
	                                                     : MOUSEEVENTF_MIDDLEDOWN))

#endif

#if defined(IS_MACOSX)
/**
 * Calculate the delta for a mouse move and add them to the event.
 * @param event The mouse move event (by ref).
 * @param point The new mouse x and y.
 */
void calculateDeltas(CGEventRef *event, MMPoint point)
{
	/**
	 * The next few lines are a workaround for games not detecting mouse moves.
	 * See this issue for more information:
	 * https://github.com/octalmage/robotjs/issues/159
	 */
	CGEventRef get = CGEventCreate(NULL);
	CGPoint mouse = CGEventGetLocation(get);

	// Calculate the deltas.
	int64_t deltaX = point.x - mouse.x;
	int64_t deltaY = point.y - mouse.y;

	CGEventSetIntegerValueField(*event, kCGMouseEventDeltaX, deltaX);
	CGEventSetIntegerValueField(*event, kCGMouseEventDeltaY, deltaY);

	CFRelease(get);
}
#endif


/**
 * Move the mouse to a specific point.
 * @param point The coordinates to move the mouse to (x, y).
 */
void moveMouse(MMPoint point)
{
#ifdef USE_X11
	Display *display = XGetMainDisplay();
	XWarpPointer(display, None, DefaultRootWindow(display),
	             0, 0, 0, 0, point.x, point.y);
	XSync(display, false);
#endif
#ifdef IS_MACOSX
	CGPoint position = CGPointMake (point.x, point.y);
	// Create an HID hardware event source
	CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);

	CGEventRef evt = NULL;
	if (CGEventSourceButtonState (kCGEventSourceStateHIDSystemState, kCGMouseButtonLeft)) {
		// Create a left button drag
		evt = CGEventCreateMouseEvent
			(src, kCGEventLeftMouseDragged,
			 position, kCGMouseButtonLeft);
	} else {
		if (CGEventSourceButtonState (kCGEventSourceStateHIDSystemState, kCGMouseButtonRight)) {
			// Create a right button drag
			evt = CGEventCreateMouseEvent
				(src, kCGEventRightMouseDragged,
				 position, kCGMouseButtonLeft);
		} else {
			// Create a mouse move event
			evt = CGEventCreateMouseEvent
				(src, kCGEventMouseMoved,
				 position, kCGMouseButtonLeft);
		}
	}

	// Post mouse event and release
	CGEventPost (kCGHIDEventTap, evt);
	if (evt != NULL) {
		CFRelease (evt);
	}
	CFRelease (src);
#endif
#ifdef IS_WINDOWS
	SetCursorPos (point.x, point.y);
#endif
}

void dragMouse(MMPoint point, const MMMouseButton button)
{
#if defined(IS_MACOSX)
	CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
	const CGEventType dragType = MMMouseDragToCGEventType(button);
	CGEventRef drag = CGEventCreateMouseEvent(src, dragType,
	                                                CGPointFromMMPoint(point),
	                                                (CGMouseButton)button);
	calculateDeltas(&drag, point);

	CGEventPost(kCGHIDEventTap, drag);
	CFRelease(drag);
	CFRelease(src);
#else
	moveMouse(point);
#endif
}

MMPoint getMousePos()
{
#if defined(IS_MACOSX)
	CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
	CGEventRef event = CGEventCreate(src);
	CGPoint point = CGEventGetLocation(event);
	CFRelease(event);
	CFRelease(src);

	return MMPointFromCGPoint(point);
#elif defined(USE_X11)
	int x, y; /* This is all we care about. Seriously. */
	Window garb1, garb2; /* Why you can't specify NULL as a parameter */
	int garb_x, garb_y;  /* is beyond me. */
	unsigned int more_garbage;

	Display *display = XGetMainDisplay();
	XQueryPointer(display, XDefaultRootWindow(display), &garb1, &garb2,
	              &x, &y, &garb_x, &garb_y, &more_garbage);

	return MMPointMake(x, y);
#elif defined(IS_WINDOWS)
	POINT point;
	GetCursorPos(&point);

	return MMPointFromPOINT(point);
#endif
}

/**
 * Press down a button, or release it.
 * @param down   True for down, false for up.
 * @param button The button to press down or release.
 */
void toggleMouse(bool down, MMMouseButton button)
{
#if defined(IS_MACOSX)
	const CGPoint currentPos = CGPointFromMMPoint(getMousePos());
	const CGEventType mouseType = MMMouseToCGEventType(down, button);
	CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
	CGEventRef event = CGEventCreateMouseEvent(src,
	                                           mouseType,
	                                           currentPos,
	                                           (CGMouseButton)button);
	CGEventPost(kCGHIDEventTap, event);
	CFRelease(event);
	CFRelease(src);
#elif defined(USE_X11)
	Display *display = XGetMainDisplay();
	XTestFakeButtonEvent(display, button, down ? True : False, CurrentTime);
	XSync(display, false);
#elif defined(IS_WINDOWS)
	INPUT mouseInput;
	mouseInput.type = INPUT_MOUSE;
	mouseInput.mi.mouseData = 0;
	mouseInput.mi.dx = 0;
	mouseInput.mi.dy = 0;
	mouseInput.mi.time = 0;
	mouseInput.mi.dwFlags = MMMouseToMEventF(down, button);
	SendInput(1, &mouseInput, sizeof(mouseInput));
#endif
}

void clickMouse(MMMouseButton button)
{
	toggleMouse(true, button);
	toggleMouse(false, button);
}

/**
 * Special function for sending double clicks, needed for Mac OS X.
 * @param button Button to click.
 */
void doubleClick(MMMouseButton button)
{

#if defined(IS_MACOSX)

	/* Double click for Mac. */
	const CGPoint currentPos = CGPointFromMMPoint(getMousePos());
	const CGEventType mouseTypeDown = MMMouseToCGEventType(true, button);
	const CGEventType mouseTypeUP = MMMouseToCGEventType(false, button);

	CGEventSourceRef src = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
	CGEventRef event = CGEventCreateMouseEvent(src, mouseTypeDown, currentPos, kCGMouseButtonLeft);

	/* Set event to double click. */
	CGEventSetIntegerValueField(event, kCGMouseEventClickState, 2);

	CGEventPost(kCGHIDEventTap, event);

	CGEventSetType(event, mouseTypeUP);
	CGEventPost(kCGHIDEventTap, event);

	CFRelease(event);
	CFRelease(src);

#else

	/* Double click for everything else. */
	clickMouse(button);
	microsleep(200);
	clickMouse(button);

#endif
}

void scrollMouse(int x, int y)
{
#if defined(IS_WINDOWS)
	// Fix for #97 https://github.com/octalmage/robotjs/issues/97,
	// C89 needs variables declared on top of functions (mouseScrollInput)
	INPUT mouseScrollInputH;
	INPUT mouseScrollInputV;
#endif

  /* Direction should only be considered based on the scrollDirection. This
   * Should not interfere. */

  /* Set up the OS specific solution */
#if defined(__APPLE__)

	CGEventRef event;

	event = CGEventCreateScrollWheelEvent(NULL, kCGScrollEventUnitPixel, 2, y, x);
	CGEventPost(kCGHIDEventTap, event);

	CFRelease(event);

#elif defined(USE_X11)

	/*
	X11 Mouse Button Numbering
	1 = left button
	2 = middle button (pressing the scroll wheel)
	3 = right button
	4 = turn scroll wheel up
	5 = turn scroll wheel down
	6 = push scroll wheel left
	7 = push scroll wheel right
	8 = 4th button (aka browser backward button)
	9 = 5th button (aka browser forward button)
	*/
	int ydir = 4; // Button 4 is up, 5 is down.
	int xdir = 6; // Button 6 is left, 7 is right.
	Display *display = XGetMainDisplay();

	if (y < 0){
		ydir = 5;
	}
	if (x < 0){
		xdir = 7;
	}

	int xi;
	int yi;
	for (xi = 0; xi < abs(x); xi++) {
		XTestFakeButtonEvent(display, xdir, 1, CurrentTime);
		XTestFakeButtonEvent(display, xdir, 0, CurrentTime);
	}
	for (yi = 0; yi < abs(y); yi++) {
		XTestFakeButtonEvent(display, ydir, 1, CurrentTime);
		XTestFakeButtonEvent(display, ydir, 0, CurrentTime);
	}

	XSync(display, false);

#elif defined(IS_WINDOWS)

	mouseScrollInputH.type = INPUT_MOUSE;
	mouseScrollInputH.mi.dx = 0;
	mouseScrollInputH.mi.dy = 0;
	mouseScrollInputH.mi.dwFlags = MOUSEEVENTF_HWHEEL;
	mouseScrollInputH.mi.time = 0;
	mouseScrollInputH.mi.dwExtraInfo = 0;
	// Flip x to match other platforms.
	mouseScrollInputH.mi.mouseData = -x;

	mouseScrollInputV.type = INPUT_MOUSE;
	mouseScrollInputV.mi.dx = 0;
	mouseScrollInputV.mi.dy = 0;
	mouseScrollInputV.mi.dwFlags = MOUSEEVENTF_WHEEL;
	mouseScrollInputV.mi.time = 0;
	mouseScrollInputV.mi.dwExtraInfo = 0;
	mouseScrollInputV.mi.mouseData = y;

	SendInput(1, &mouseScrollInputH, sizeof(mouseScrollInputH));
	SendInput(1, &mouseScrollInputV, sizeof(mouseScrollInputV));
#endif
}