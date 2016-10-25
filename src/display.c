/*
 * display.c
 * Copyright (C) 2016 Alexei Frolov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include "display.h"
#include "kbm.h"
#include "keymap.h"
#include "hotkey.h"

#if defined(__linux__) || defined(__APPLE__)
#define MAX_PATH 4096

#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif /* __linux__ || __APPLE__ */

/* all hotkey mappings excluding toggles */
static struct hotkey *actions;
/* toggle hotkey mappings */
static struct hotkey *toggles;

static void map_keys(struct hotkey *head);
static void unmap_keys(struct hotkey *head);
static struct hotkey *find_by_os_code(struct hotkey *head,
				      uint32_t code, uint32_t mask);
static void send_notification(const char *msg);


#ifdef __linux__
#include <libnotify/notify.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xtest.h>

/* connection to the X server */
static xcb_connection_t *conn;

/* root screen of the X display and root window of screen */
static xcb_screen_t *root_screen;
static xcb_window_t root;

/* X11 keysyms */
static xcb_key_symbols_t *keysyms;

static int isnummod(unsigned int keysym);

/* init_display: connect to the X server and grab the root window */
int init_display(void)
{
	int screen;

	if (!(conn = xcb_connect(NULL, &screen))) {
		fprintf(stderr, "error: failed to connect to X server\n");
		return 1;
	}
	/* get the root screen and root window of the X display */
	root_screen = xcb_aux_get_screen(conn, screen);
	root = root_screen->root;
	keysyms = xcb_key_symbols_alloc(conn);

	actions = toggles = NULL;

	if (kbm_info.notifications)
		notify_init(PROGRAM_NAME);

	return 0;
}

/* close_display: disconnect from X server and clean up */
void close_display(void)
{
	unmap_keys(actions);
	unmap_keys(toggles);
	xcb_key_symbols_free(keysyms);
	xcb_disconnect(conn);

	if (kbm_info.notifications)
		notify_uninit();
}

/*
 * A key press event that occurs at the same time as a previous
 * key release with the same key is an automatically repeated key.
 */
#define DETECT_AUTOREPEAT(last, evt, ks) \
	(last && (last->response_type & ~0x80) == XCB_KEY_RELEASE \
	 && (last_ks) == (ks) && last->time == evt->time)

/* start_listening: map all hotkeys and start listening for keypresses */
void start_listening(void)
{
	xcb_generic_event_t *e;
	xcb_key_press_event_t *evt, *last;
	xcb_keysym_t ks, last_ks;
	struct hotkey *hk;
	unsigned int running = 1;

	last = NULL;
	while (running && (e = xcb_wait_for_event(conn))) {
		switch (e->response_type & ~0x80) {
		case XCB_KEY_PRESS:
			evt = (xcb_key_press_event_t *)e;
			ks = xcb_key_press_lookup_keysym(keysyms, evt, 0);

			/*
			 * If the key is not a numpad key, unset the Num Lock
			 * bit as it is irrelevant. If it is a numpad key, the
			 * Num Lock bit differentiates between the key's two
			 * functions.
			 */
			if (!isnummod(ks))
				evt->state &= ~XCB_MOD_MASK_2;
			/* unset the caps lock bit for every key */
			evt->state &= ~XCB_MOD_MASK_LOCK;

			if (!(hk = find_by_os_code(actions, ks, evt->state))
					&& !(hk = find_by_os_code(toggles,
							ks, evt->state))) {
				/*
				 * This sometimes happens when keys are
				 * pressed in quick succession.
				 * The event should be sent back out.
				 */
				break;
			}

			/* don't send an autorepeated key if norepeat flag */
			if (DETECT_AUTOREPEAT(last, evt, ks) &&
					CHECK_MASK(hk->key_flags, KBM_NOREPEAT))
				break;

			if (process_hotkey(hk, KBM_PRESS) == -1)
				running = 0;
			break;
		case XCB_KEY_RELEASE:
			evt = (xcb_key_press_event_t *)e;
			ks = xcb_key_press_lookup_keysym(keysyms, evt, 0);

			if (!isnummod(ks))
				evt->state &= ~XCB_MOD_MASK_2;
			evt->state &= ~XCB_MOD_MASK_LOCK;

			if (!(hk = find_by_os_code(actions, ks, evt->state))
					&& !(hk = find_by_os_code(toggles,
							ks, evt->state)))
				break;

			process_hotkey(hk, KBM_RELEASE);
			break;
		default:
			continue;
		}
		free(last);
		last = evt;
		last_ks = ks;
	}
	free(e);
}

/* send_button: send a button event */
void send_button(unsigned int button)
{
	xcb_test_fake_input(conn, XCB_BUTTON_PRESS, button,
			XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
	xcb_test_fake_input(conn, XCB_BUTTON_RELEASE, button,
			XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
	xcb_flush(conn);
}

/* send_key: send a key event */
void send_key(unsigned int keycode, unsigned int modmask, unsigned int type)
{
	xcb_keycode_t *kc, *mod;

	kc = xcb_key_symbols_get_keycode(keysyms, keycode);
	mod = NULL;
	if (type == KBM_PRESS) {
		/* press all required modifier keys */
		if (CHECK_MASK(modmask, XCB_MOD_MASK_SHIFT)) {
			mod = xcb_key_symbols_get_keycode(keysyms, XK_Shift_L);
			xcb_test_fake_input(conn, XCB_KEY_PRESS, mod[0],
					XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
		}
		if (CHECK_MASK(modmask, XCB_MOD_MASK_CONTROL)) {
			mod = xcb_key_symbols_get_keycode(keysyms, XK_Control_L);
			xcb_test_fake_input(conn, XCB_KEY_PRESS, mod[0],
					XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
		}
		if (CHECK_MASK(modmask, XCB_MOD_MASK_4)) {
			mod = xcb_key_symbols_get_keycode(keysyms, XK_Super_L);
			xcb_test_fake_input(conn, XCB_KEY_PRESS, mod[0],
					XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
		}
		if (CHECK_MASK(modmask, XCB_MOD_MASK_1)) {
			mod = xcb_key_symbols_get_keycode(keysyms, XK_Alt_L);
			xcb_test_fake_input(conn, XCB_KEY_PRESS, mod[0],
					XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
		}
		/* press the requested key */
		xcb_test_fake_input(conn, XCB_KEY_PRESS, kc[0],
				XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
	} else {
		/* release the requested keys and then all modifiers */
		xcb_test_fake_input(conn, XCB_KEY_RELEASE, kc[0],
				XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
		if (CHECK_MASK(modmask, XCB_MOD_MASK_SHIFT)) {
			mod = xcb_key_symbols_get_keycode(keysyms, XK_Shift_L);
			xcb_test_fake_input(conn, XCB_KEY_RELEASE, mod[0],
					XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
		}
		if (CHECK_MASK(modmask, XCB_MOD_MASK_CONTROL)) {
			mod = xcb_key_symbols_get_keycode(keysyms, XK_Control_L);
			xcb_test_fake_input(conn, XCB_KEY_RELEASE, mod[0],
					XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
		}
		if (CHECK_MASK(modmask, XCB_MOD_MASK_4)) {
			mod = xcb_key_symbols_get_keycode(keysyms, XK_Super_L);
			xcb_test_fake_input(conn, XCB_KEY_RELEASE, mod[0],
					XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
		}
		if (CHECK_MASK(modmask, XCB_MOD_MASK_1)) {
			mod = xcb_key_symbols_get_keycode(keysyms, XK_Alt_L);
			xcb_test_fake_input(conn, XCB_KEY_RELEASE, mod[0],
					XCB_CURRENT_TIME, XCB_NONE, 0, 0, 0);
		}
		xcb_flush(conn);
	}
	free(kc);
	if (mod)
		free(mod);
}

/* move_cursor: move cursor along vector x,y from current position */
void move_cursor(int x, int y)
{
	xcb_warp_pointer(conn, XCB_NONE, XCB_NONE, 0, 0, 0, 0, x, y);
	xcb_flush(conn);
}

/* map_keys: grab all provided hotkeys */
static void map_keys(struct hotkey *head)
{
	xcb_keycode_t *kc;
	xcb_void_cookie_t cookie;
	xcb_generic_error_t *err;

	if (head && head->op != OP_TOGGLE)
		kbm_info.keys_active = 1;

	for (; head; head = head->next) {
		kc = xcb_key_symbols_get_keycode(keysyms, head->os_code);
		cookie = xcb_grab_key_checked(conn, 1, root,
				head->os_modmask, kc[0],
				XCB_GRAB_MODE_ASYNC,
				XCB_GRAB_MODE_ASYNC);

		/* key grab will fail if the key is already grabbed */
		if ((err = xcb_request_check(conn, cookie))) {
			fprintf(stderr, "error: the key '%s' is already "
					"mapped by another program\n",
					keystr(head->kbm_code,
					       head->kbm_modmask));
			free(err);
		}

		/*
		 * In X11, Caps Lock and Num Lock are defined as modifiers and
		 * events involving these keys held down are treated as
		 * different events to those occurring without them.
		 *
		 * We don't want to distinguish between these events, so we
		 * also grab the key with the Caps and Num Lock masks.
		 */

		/* num lock */
		xcb_grab_key(conn, 1, root, head->os_modmask | XCB_MOD_MASK_2,
				kc[0], XCB_GRAB_MODE_ASYNC,
				XCB_GRAB_MODE_ASYNC);
		/* caps lock */
		xcb_grab_key(conn, 1, root, head->os_modmask
				| XCB_MOD_MASK_LOCK, kc[0],
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
		/* both */
		xcb_grab_key(conn, 1, root, head->os_modmask
				| XCB_MOD_MASK_LOCK | XCB_MOD_MASK_2, kc[0],
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
		free(kc);
	}
	xcb_flush(conn);
}

/* unmap_keys: ungrab all assigned hotkeys */
static void unmap_keys(struct hotkey *head)
{
	xcb_keycode_t *kc;

	if (head && head->op != OP_TOGGLE)
		kbm_info.keys_active = 0;

	for (; head; head = head->next) {

		kc = xcb_key_symbols_get_keycode(keysyms, head->os_code);
		xcb_ungrab_key(conn, kc[0], root, head->os_modmask);

		/* account for num lock and caps lock modifiers */
		xcb_ungrab_key(conn, kc[0], root, head->os_modmask
				| XCB_MOD_MASK_2);
		xcb_ungrab_key(conn, kc[0], root, head->os_modmask
				| XCB_MOD_MASK_LOCK);
		xcb_ungrab_key(conn, kc[0], root, head->os_modmask
				| XCB_MOD_MASK_2 | XCB_MOD_MASK_LOCK);
	}
	xcb_flush(conn);
}

/* isnummod: check if a key is modifiable through num lock */
static int isnummod(unsigned int keysym)
{
	switch (keysym) {
	case XK_KP_Delete:
	case XK_KP_Insert:
	case XK_KP_End:
	case XK_KP_Down:
	case XK_KP_Next:
	case XK_KP_Left:
	case XK_KP_Begin:
	case XK_KP_Right:
	case XK_KP_Home:
	case XK_KP_Up:
	case XK_KP_Prior:
		return 1;
	default:
		return 0;
	}
}

static void send_notification(const char *msg)
{
	NotifyNotification *n;
	GError *err;

	err = NULL;
	n = notify_notification_new(msg, NULL, NULL);
	if (!notify_notification_show(n, &err)) {
		fprintf(stderr, "failed to send notification: %s\n",
				err->message);
		g_error_free(err);
	}
	g_object_unref(G_OBJECT(n));
}
#endif /* __linux__ */


#if defined(__CYGWIN__) || defined (__MINGW32__)
#include <Windows.h>

#define KBM_UID 38471
#define CLASS_NAME "KBM_WINDOW"

/* context menu options */
enum {
	KBM_MENU_QUIT = 0x800,
	KBM_MENU_NOTIFY
};

/* hook for keyboard input */
HHOOK hook;

HWND kbm_window;

/*
 * Track fake modifier keypresses and releases sent by the program.
 * These are ignored when looking up active modifiers during a key release.
 */
static int fake_mods[4] = { 0, 0, 0, 0 };

static LRESULT CALLBACK kbproc(int nCode, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK wndproc(HWND hWnd, UINT uMsg,
				WPARAM wParam, LPARAM lParam);
static unsigned int numpad_keycode(unsigned int kc);
static void check_modifiers(unsigned int *mods);
static void unset_fake_mods(unsigned int *mods);
static void send_fake_mod(unsigned int keycode, int type);
static void kill_fake_mods(void);
static void show_context_menu(void);

/*
 * init_display:
 * Create a window and system tray icon for the program.
 * Set up keyboard event hook.
 */
int init_display(void)
{
	WNDCLASSEX wx;
	NOTIFYICONDATA n;

	memset(&wx, 0, sizeof(wx));
	wx.cbSize = sizeof(wx);
	wx.lpfnWndProc = wndproc;
	wx.lpszClassName = CLASS_NAME;
	wx.hInstance = kbm_info.instance;

	if (!RegisterClassEx(&wx)) {
		fprintf(stderr, "error: failed to register window class\n");
		return 1;
	}

	kbm_window = CreateWindowEx(0, CLASS_NAME, "kbm", 0, 0,
				    0, 0, 0, HWND_MESSAGE, NULL,
				    kbm_info.instance, NULL);
	if (!kbm_window) {
		fprintf(stderr, "error: failed to create main window\n");
		goto err_window;
	}

	memset(&n, 0, sizeof(n));
	n.cbSize = sizeof(n);
	n.hWnd = kbm_window;
	n.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	n.dwState = NIS_SHAREDICON;
	n.uID = KBM_UID;
	n.uCallbackMessage = WM_APP;
	n.hIcon = LoadImage(kbm_info.instance, MAKEINTRESOURCE(0),
			    IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	strcpy(n.szTip, "kbm");

	Shell_NotifyIcon(NIM_ADD, &n);

	if (!(hook = SetWindowsHookEx(WH_KEYBOARD_LL, kbproc, NULL, 0))) {
		fprintf(stderr, "error: failed to set keyboard hook\n");
		goto err_hook;
	}

	return 0;

err_hook:
	Shell_NotifyIcon(NIM_DELETE, &n);
	DestroyWindow(kbm_window);
err_window:
	UnregisterClass(CLASS_NAME, NULL);
	return 1;
}

void close_display(void)
{
	NOTIFYICONDATA n;

	n.cbSize = sizeof(n);
	n.hWnd = kbm_window;
	n.uID = KBM_UID;

	Shell_NotifyIcon(NIM_DELETE, &n);
	DestroyWindow(kbm_window);
	UnregisterClass(CLASS_NAME, NULL);
	UnhookWindowsHookEx(hook);
}

/* start_listening: start listening for keypresses */
void start_listening(void)
{
	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void send_button(unsigned int button)
{
	INPUT ip;
	unsigned int dnflags, upflags;

	ip.type = INPUT_MOUSE;
	memset(&ip.mi, 0, sizeof(ip.mi));

	switch (button) {
	case KBM_BUTTON_LEFT:
		dnflags = MOUSEEVENTF_LEFTDOWN;
		upflags = MOUSEEVENTF_LEFTUP;
		break;
	case KBM_BUTTON_MIDDLE:
		dnflags = MOUSEEVENTF_MIDDLEDOWN;
		upflags = MOUSEEVENTF_MIDDLEUP;
		break;
	case KBM_BUTTON_RIGHT:
		dnflags = MOUSEEVENTF_RIGHTDOWN;
		upflags = MOUSEEVENTF_RIGHTUP;
		break;
	default:
		return;
	}

	ip.mi.dwFlags = dnflags;
	SendInput(1, &ip, sizeof(ip));
	ip.mi.dwFlags = upflags;
	SendInput(1, &ip, sizeof(ip));
}

/* send_key: send a key event */
void send_key(unsigned int keycode, unsigned int modmask, unsigned int type)
{
	INPUT key;

	/*
	 * If the key is a modifier, it is sent as a fake modifier so that
	 * it can be distinguished from physically held modifier keys.
	 */
	if (keycode == VK_SHIFT || keycode == VK_CONTROL
			|| keycode == VK_MENU || keycode == VK_LWIN) {
		send_fake_mod(keycode, type);
		return;
	}

	key.type = INPUT_KEYBOARD;
	memset(&key.ki, 0, sizeof(key.ki));
	key.ki.wVk = keycode;

	if (type == KBM_RELEASE)
		key.ki.dwFlags = KEYEVENTF_KEYUP;

	if (type == KBM_RELEASE)
		SendInput(1, &key, sizeof(key));

	if (CHECK_MASK(modmask, MOD_SHIFT))
		send_fake_mod(VK_SHIFT, type);
	if (CHECK_MASK(modmask, MOD_CONTROL))
		send_fake_mod(VK_CONTROL, type);
	if (CHECK_MASK(modmask, MOD_ALT))
		send_fake_mod(VK_MENU, type);
	if (CHECK_MASK(modmask, MOD_WIN))
		send_fake_mod(VK_LWIN, type);

	if (type == KBM_PRESS)
		SendInput(1, &key, sizeof(key));
}

/* move_cursor: move cursor along vector x,y from current position */
void move_cursor(int x, int y)
{
	POINT pt;

	GetCursorPos(&pt);
	SetCursorPos(pt.x + x, pt.y + y);
}

/* kbm_exec: execute the specified program */
void kbm_exec(void *args)
{
	char *cmd;
	char err[256];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);

	/*
	 * The CreateProcess function can modify the string
	 * passed to it, so we create a copy of args to use.
	 */
	cmd = strdup(args);
	if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE,
				0, NULL, NULL, &si, &pi)) {
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				err, 256, NULL);
		fprintf(stderr, "%s\n", err);
		return;
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	free(cmd);
}

/* kbproc: process a keyboard event */
static LRESULT CALLBACK kbproc(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT *kb;
	unsigned int kc, mods;
	struct hotkey *hk;

	if (nCode != HC_ACTION)
		return CallNextHookEx(hook, nCode, wParam, lParam);

	kb = (KBDLLHOOKSTRUCT *)lParam;
	kc = kb->vkCode;
	/* return key sent with lowest bit of flags set is NumEnter */
	if (kc == VK_RETURN && kb->flags & 1)
		kc = 0x6C;

	/* differentiate between numpad keys with numlock off and normal keys */
	if (kc >= VK_PRIOR && kc <= VK_DELETE) {
		if (!(kb->flags & 1))
			kc = numpad_keycode(kc);
	}

	check_modifiers(&mods);

	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
		if (kbm_info.keys_active && (hk = find_by_os_code(
						actions, kc, mods))) {
			if (process_hotkey(hk, KBM_PRESS) == -1) {
				/*
				 * Any fake modifiers in the down position
				 * when the program exits should be lifted.
				 */
				kill_fake_mods();
				PostQuitMessage(0);
			}
			/* prevent the event from propagating further */
			return 1;
		}
		if ((hk = find_by_os_code(toggles, kc, mods))) {
			process_hotkey(hk, KBM_PRESS);
			return 1;
		}
	} else {
		/*
		 * Fake modifiers sent by the program should
		 * be ignored when keys are released.
		 */
		unset_fake_mods(&mods);
		if (kbm_info.keys_active && (hk = find_by_os_code(
						actions, kc, mods))) {
			process_hotkey(hk, KBM_RELEASE);
			return 1;
		}
	}

	return CallNextHookEx(hook, nCode, wParam, lParam);
}

static LRESULT CALLBACK wndproc(HWND hWnd, UINT uMsg,
				WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_APP:
		if (lParam == WM_RBUTTONUP)
			show_context_menu();
		return 0;
	case WM_COMMAND:
		switch (wParam & 0xFFFF) {
		case KBM_MENU_QUIT:
			PostQuitMessage(0);
			break;
		case KBM_MENU_NOTIFY:
			kbm_info.notifications = !kbm_info.notifications;
			break;
		}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

}

static unsigned int numpad_keycode(unsigned int kc)
{
	switch (kc) {
	case VK_DELETE:
		return kbm_to_win32(KEY_NUMDEL);
	case VK_INSERT:
		return kbm_to_win32(KEY_NUMINS);
	case VK_END:
		return kbm_to_win32(KEY_NUMEND);
	case VK_DOWN:
		return kbm_to_win32(KEY_NUMDOWN);
	case VK_NEXT:
		return kbm_to_win32(KEY_NUMPGDN);
	case VK_LEFT:
		return kbm_to_win32(KEY_NUMLEFT);
	case VK_RIGHT:
		return kbm_to_win32(KEY_NUMRIGHT);
	case VK_HOME:
		return kbm_to_win32(KEY_NUMHOME);
	case VK_UP:
		return kbm_to_win32(KEY_NUMUP);
	case VK_PRIOR:
		return kbm_to_win32(KEY_NUMPGUP);
	default:
		return kc;
	}
}

/* check_modifiers: check active modifiers and set mods to their bitmasks */
static void check_modifiers(unsigned int *mods)
{
	*mods = 0;
	if (GetKeyState(VK_SHIFT) & 0x8000)
		*mods |= MOD_SHIFT;
	if (GetKeyState(VK_CONTROL) & 0x8000)
		*mods |= MOD_CONTROL;
	if (GetKeyState(VK_MENU) & 0x8000)
		*mods |= MOD_ALT;
	if (GetKeyState(VK_LWIN) & 0x8000 || GetKeyState(VK_RWIN) & 0x8000)
		*mods |= MOD_WIN;
}

/* unset_fake_mods: remove modifier masks of active fake modifiers from mods */
static void unset_fake_mods(unsigned int *mods)
{
	if (fake_mods[0])
		*mods &= ~MOD_SHIFT;
	if (fake_mods[1])
		*mods &= ~MOD_CONTROL;
	if (fake_mods[2])
		*mods &= ~MOD_ALT;
	if (fake_mods[3])
		*mods &= ~MOD_WIN;
}

/* send_fake_mod: send a fake modifier key event */
static void send_fake_mod(unsigned int keycode, int type)
{
	INPUT mod;
	size_t i;

	mod.type = INPUT_KEYBOARD;
	memset(&mod.ki, 0, sizeof(mod.ki));
	mod.ki.wVk = keycode;

	switch (keycode) {
	case VK_SHIFT:
		i = 0;
		break;
	case VK_CONTROL:
		i = 1;
		break;
	case VK_MENU:
		i = 2;
		break;
	case VK_LWIN:
		i = 3;
		break;
	default:
		return;
	}

	if (type == KBM_RELEASE)
		mod.ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(1, &mod, sizeof(mod));
	fake_mods[i] = type == KBM_PRESS;
}

/* kill_fake_mods: release all fake modifers that are active */
static void kill_fake_mods(void)
{
	if (fake_mods[0])
		send_fake_mod(VK_SHIFT, KBM_RELEASE);
	if (fake_mods[1])
		send_fake_mod(VK_CONTROL, KBM_RELEASE);
	if (fake_mods[2])
		send_fake_mod(VK_MENU, KBM_RELEASE);
	if (fake_mods[3])
		send_fake_mod(VK_LWIN, KBM_RELEASE);
}

static void map_keys(struct hotkey *head)
{
	if (head && head->op != OP_TOGGLE)
		kbm_info.keys_active = 1;
}

static void unmap_keys(struct hotkey *head)
{
	if (head && head->op != OP_TOGGLE)
		kbm_info.keys_active = 0;
}

static void send_notification(const char *msg)
{
	NOTIFYICONDATA n;

	memset(&n, 0, sizeof(n));
	n.cbSize = sizeof(n);
	n.hWnd = kbm_window;
	n.uFlags = NIF_TIP | NIF_INFO;
	n.uID = KBM_UID;
	strcpy(n.szTip, "kbm");
	strcpy(n.szInfo, msg);

	Shell_NotifyIcon(NIM_MODIFY, &n);
}

/*
 * show_context_menu:
 * Create a context menu at the current cursor position.
 * Send a message to window with the user's choice.
 */
static void show_context_menu(void)
{
	HMENU menu;
	POINT pt;
	int check;

	check = kbm_info.notifications ? MF_CHECKED : MF_UNCHECKED;

	menu = CreatePopupMenu();
	InsertMenu(menu, 0, MF_BYPOSITION | MF_STRING | check,
			KBM_MENU_NOTIFY, "Notifications");
	InsertMenu(menu, 1, MF_BYPOSITION | MF_STRING,
			KBM_MENU_QUIT, "Quit");

	GetCursorPos(&pt);
	SetForegroundWindow(kbm_window);

	TrackPopupMenuEx(menu, TPM_LEFTALIGN | TPM_BOTTOMALIGN |
			TPM_RIGHTBUTTON, pt.x, pt.y, kbm_window, NULL);

	DestroyMenu(menu);
}
#endif /* __CYGWIN__ || __MINGW32__ */


#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include "application.h"

static CGEventRef callback(CGEventTapProxy proxy, CGEventType type,
			   CGEventRef event, void *refcon);
static int open_app(char **argv);

/* init_display: enable the keypress event tap */
int init_display(void)
{
	CFMachPortRef tap;
	CGEventMask mask;
	CFRunLoopSourceRef src;

	mask = (1 << kCGEventKeyDown) | (1 << kCGEventKeyUp);
	tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap,
			0, mask, callback, NULL);
	if (!tap) {
		/* enable access for assistive devices */
		fprintf(stderr, "error: failed to create event tap\n");
		osx_alert("test");
		return 1;
	}

	src = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), src, kCFRunLoopCommonModes);
	CGEventTapEnable(tap, true);

	return 0;
}

void close_display(void)
{
}

void start_listening(void)
{
	CFRunLoopRun();
}

void send_button(unsigned int button)
{
	CGEventRef posevent, downevent, upevent;
	CGPoint pos;
	CGEventType dtype, utype;
	CGMouseButton mb;

	/* get the current position of the cursor by making an empty event */
	posevent = CGEventCreate(NULL);
	pos = CGEventGetLocation(posevent);

	switch (button) {
	case KBM_BUTTON_LEFT:
		dtype = kCGEventLeftMouseDown;
		utype = kCGEventLeftMouseUp;
		mb = kCGMouseButtonLeft;
		break;
	case KBM_BUTTON_MIDDLE:
		dtype = kCGEventOtherMouseDown;
		utype = kCGEventOtherMouseUp;
		mb = kCGMouseButtonCenter;
		break;
	case KBM_BUTTON_RIGHT:
		dtype = kCGEventRightMouseDown;
		utype = kCGEventRightMouseUp;
		mb = kCGMouseButtonRight;
		break;
	default:
		return;
	}

	downevent = CGEventCreateMouseEvent(NULL, dtype, pos, mb);
	upevent = CGEventCreateMouseEvent(NULL, utype, pos, mb);

	CGEventPost(kCGHIDEventTap, downevent);
	CGEventPost(kCGHIDEventTap, upevent);

	CFRelease(posevent);
	CFRelease(downevent);
	CFRelease(upevent);
}

/* send_key: send a key event */
void send_key(unsigned int keycode, unsigned int modmask, unsigned int type)
{
	CGEventRef key;

	if (type == KBM_PRESS) {
		key = CGEventCreateKeyboardEvent(NULL, keycode, true);
		CGEventSetFlags(key, modmask);
		CGEventPost(kCGHIDEventTap, key);
	} else {
		key = CGEventCreateKeyboardEvent(NULL, keycode, false);
		CGEventSetFlags(key, modmask);
		CGEventPost(kCGHIDEventTap, key);
	}

	CFRelease(key);
}

/* move_cursor: move cursor along vector x,y from current position */
void move_cursor(int x, int y)
{
	CGEventRef posevent, moveevent;
	CGPoint pos;

	/* get cursor position */
	posevent = CGEventCreate(NULL);
	pos = CGEventGetLocation(posevent);

	pos.x += x;
	pos.y += y;
	moveevent = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, pos, 0);
	CGEventPost(kCGHIDEventTap, moveevent);

	CFRelease(posevent);
	CFRelease(moveevent);
}

static void map_keys(struct hotkey *head)
{
	if (head && head->op != OP_TOGGLE)
		kbm_info.keys_active = 1;
}

static void unmap_keys(struct hotkey *head)
{
	if (head && head->op != OP_TOGGLE)
		kbm_info.keys_active = 0;
}

/* callback: function called when event is registered */
static CGEventRef callback(CGEventTapProxy proxy, CGEventType type,
			   CGEventRef event, void *refcon)
{
	CGKeyCode keycode;
	CGEventFlags flags;
	struct hotkey *hk;

	KBM_UNUSED(proxy);
	KBM_UNUSED(refcon);

	/* just in case */
	if (type != kCGEventKeyDown && type != kCGEventKeyUp)
		return event;

	keycode = (CGKeyCode)CGEventGetIntegerValueField(event,
			kCGKeyboardEventKeycode);
	flags = CGEventGetFlags(event);
	/* filter out all the bits we're not interested in */
	flags &= (kCGEventFlagMaskShift | kCGEventFlagMaskControl
			| kCGEventFlagMaskCommand | kCGEventFlagMaskAlternate);

	if (type == kCGEventKeyDown) {
		if (kbm_info.keys_active && (hk = find_by_os_code(actions,
							keycode, flags))) {
			if (process_hotkey(hk, KBM_PRESS) == -1) {
				CFRunLoopStop(CFRunLoopGetCurrent());
				terminate_app();
			}
			/* prevent the event from propagating further */
			return NULL;
		}
		if ((hk = find_by_os_code(toggles, keycode, flags))) {
			process_hotkey(hk, KBM_PRESS);
			return NULL;
		}
	} else {
		if (kbm_info.keys_active && (hk = find_by_os_code(actions,
							keycode, flags))) {
			process_hotkey(hk, KBM_RELEASE);
			return NULL;
		}
	}
	return event;
}

static int open_app(char **argv)
{
	int status;

	switch (fork()) {
	case -1:
		perror("fork");
		return 1;
	case 0:
		close(STDERR_FILENO);
		execv("/usr/bin/open", argv);
		perror(argv[0]);
		exit(1);
	default:
		wait(&status);
		break;
	}
	return status >> 8;
}

static void send_notification(const char *msg)
{
	osx_notify(msg);
}
#endif /* __APPLE__ */

#if defined(__linux__) || defined(__APPLE__)
/* kbm_exec: execute the specified program */
void kbm_exec(void *args)
{
	char **argv;

	argv = args;

#ifdef __APPLE__
	/*
	 * Try to open the program as an app first.
	 * If not found, treat it as a regular program.
	 */
	if (open_app(argv) == 0)
		return;

	/* jump over the 'open -a' */
	argv += 2;
#endif

	switch (fork()) {
	case -1:
		perror("fork");
		return;
	case 0:
		execvp(argv[0], argv);
		perror(argv[0]);
		exit(1);
	default:
		break;
	}
}
#endif /* __linux__ || __APPLE__ */

/* load_keys: split the keys in list head into actions and toggles */
void load_keys(struct hotkey *head)
{
	struct hotkey *tmp;

	while (head) {
		tmp = head;
		head = head->next;
		tmp->next = NULL;
		if (tmp->op == OP_TOGGLE)
			add_hotkey(&toggles, tmp);
		else
			add_hotkey(&actions, tmp);
	}

	if (kbm_info.keys_active)
		map_keys(actions);
	map_keys(toggles);
}

void unload_keys(void)
{
	if (actions)
		free_keys(actions);
	if (toggles)
		free_keys(toggles);
	actions = toggles = NULL;
}

void toggle_keys(void)
{
	if (kbm_info.keys_active) {
		unmap_keys(actions);
		if (kbm_info.notifications)
			send_notification("Hotkeys disabled");
	} else {
		map_keys(actions);
		if (kbm_info.notifications)
			send_notification("Hotkeys enabled");
	}
}

/* find_by_os_code: return the hotkey in head with os_code code */
static struct hotkey *find_by_os_code(struct hotkey *head,
				      uint32_t code, uint32_t mask)
{
	for (; head; head = head->next) {
		if (head->os_code == code && head->os_modmask == mask)
			return head;
	}
	return NULL;
}
