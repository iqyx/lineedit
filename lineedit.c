/**
 * Copyright (c) 2014, Marek Koza (qyx@krtko.org)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <inttypes.h>

#include "lineedit.h"


int u_assert_func(const char *a, const char *f, int n) {
	printf("Assertion '%s' failed in %s, line %d\n", a, f, n);
	abort();
	return 1;
}


int32_t lineedit_escape_print(struct lineedit *le, enum lineedit_escape_seq esc, int param) {
	if (u_assert(le != NULL) ||
	    u_assert(le->print_handler != NULL)) {
		return LINEEDIT_ESCAPE_PRINT_FAILED;
	}

	char s[20];
	switch (esc) {
		case ESC_CURSOR_LEFT:
			le->print_handler("\x1b[D", le->print_handler_ctx);
			break;
		case ESC_CURSOR_RIGHT:
			le->print_handler("\x1b[C", le->print_handler_ctx);
			break;
		case ESC_COLOR:
			snprintf(s, sizeof(s), "\x1b[%dm", param);
			le->print_handler(s, le->print_handler_ctx);
			break;
		case ESC_DEFAULT:
			le->print_handler("\x1b[0m", le->print_handler_ctx);
			break;
		case ESC_BOLD:
			le->print_handler("\x1b[1m", le->print_handler_ctx);
			break;
		case ESC_CURSOR_SAVE:
			le->print_handler("\x1b[s", le->print_handler_ctx);
			break;
		case ESC_CURSOR_RESTORE:
			le->print_handler("\x1b[u", le->print_handler_ctx);
			break;
		case ESC_ERASE_LINE_END:
			le->print_handler("\x1b[K", le->print_handler_ctx);
			break;
		default:
			return LINEEDIT_ESCAPE_PRINT_FAILED;
	}

	return LINEEDIT_ESCAPE_PRINT_OK;
}


int32_t lineedit_init(struct lineedit *le, uint32_t line_len) {
	if (u_assert(le != NULL) ||
	    u_assert(line_len > 0)) {
		return LINEEDIT_INIT_FAILED;
	}

	le->text = malloc(line_len);
	if (le->text == NULL) {
		return LINEEDIT_INIT_FAILED;
	}

	le->len = line_len;
	le->cursor = 0;
	le->text[0] = 0;
	le->pwchar = '\0';
	le->prompt_len = 0;
	le->csi_escape_mod = 0;
	le->print_handler = NULL;
	le->print_handler_ctx = NULL;
	le->prompt_callback = NULL;

	return LINEEDIT_INIT_OK;
}


int32_t lineedit_free(struct lineedit *le) {
	if (u_assert(le != NULL) ||
	    u_assert(le->text != NULL)) {
		return LINEEDIT_FREE_FAILED;
	}

	free(le->text);

	return LINEEDIT_FREE_OK;
}


int32_t lineedit_keypress(struct lineedit *le, int c) {
	if (u_assert(le != NULL) ||
	    u_assert(le->print_handler != NULL)) {
		return LINEEDIT_FAILED;
	}

	/* check for TAB */
	if (c == 0x09) {
		return LINEEDIT_TAB;
	}

	/* check for line feed */
	if (c == 0x0a || c == 0x0b || c == 0x0c || c == 0x0d) {
		return LINEEDIT_ENTER;
	}

	/* interrupt escape sequence */
	if (c == 0x18 || c == 0x1a) {
		le->escape = ESC_NONE;
		return LINEEDIT_OK;
	}

	/* check for ESC */
	if (c == 0x1b) {
		le->escape = ESC_ESC;
		return LINEEDIT_OK;
	}

	/* check for DEL (backspace) */
	if (c == 0x7f) {
		/* Do not check return value, if we are unable to do backspace,
		 * we just ignore it. */
		lineedit_backspace(le);
		return LINEEDIT_OK;
	}

	/* check for CSI */
	if (c == 0x9b) {
		le->escape = ESC_CSI;
		le->csi_escape_mod = 0;
		return LINEEDIT_OK;
	}

	/* if ESC is set and '[' character was received, start CSI sequence */
	if (le->escape == ESC_ESC && c == '[') {
		le->escape = ESC_CSI;
		le->csi_escape_mod = 0;
		return LINEEDIT_OK;
	}

	/* if ESC is set and ']' character was received, start OSC sequence */
	if (le->escape == ESC_ESC && c == ']') {
		le->escape = ESC_OSC;
		return LINEEDIT_OK;
	}

	/* if CSI is set, try to read first alphanumeric character (parameters are ignored */
	if (le->escape == ESC_CSI) {
		/* Escape modifier, we continue with CSI escape flag set. */
		if (c >= '0' && c <= '9') {
			le->csi_escape_mod = le->csi_escape_mod * 10 + (c - '0');
			return LINEEDIT_OK;
		}

		if (c == 'A') {
			/* Move cursor up (previous history entry). */
			/* TODO: */
		}
		if (c == 'B') {
			/* Move cursor down (next history entry). */
			/* TODO: */
		}
		if (c == 'C') {
			/* move cursor right */
			if (le->cursor < (strlen(le->text))) {
				le->cursor++;
				lineedit_escape_print(le, ESC_CURSOR_RIGHT, 1);
			}
		}
		if (c == 'D') {
			/* move cursor left */
			if (le->cursor > 0) {
				le->cursor--;
				lineedit_escape_print(le, ESC_CURSOR_LEFT, 1);
			}
		}
		if (c == '~') {
			/* Delete key. */
			lineedit_backspace(le);
		}

		le->escape = ESC_NONE;
		return LINEEDIT_OK;
	}


	/* other alphanumeric characters */
	if (c >= 32 && c <= 127) {
		/* Do not check return value, if we are unable to insert it,
		 * we just ignore the character. */
		lineedit_insert_char(le, c);
		return LINEEDIT_OK;
	}

	return LINEEDIT_OK;
}


int32_t lineedit_backspace(struct lineedit *le) {
	if (u_assert(le != NULL)) {
		return LINEEDIT_BACKSPACE_FAILED;
	}

	/* we are going to remove 1 character at cursor position,
	 * check if we have anything to remove */
	if (strlen(le->text) == 0 || le->cursor == 0) {
		return LINEEDIT_BACKSPACE_FAILED;
	}

	/* move cursor left */
	le->cursor--;
	lineedit_escape_print(le, ESC_CURSOR_LEFT, 1);

	/* shift line left */
	int32_t i = le->cursor;
	while (le->text[i] != 0) {
		le->text[i] = le->text[i + 1];
		i++;
	}

	/* save cursor position */
	lineedit_escape_print(le, ESC_CURSOR_SAVE, 0);

	/* now we need to refresh rest of the line */
	i = le->cursor;
	while (le->text[i]) {
		char line[2] = {le->text[i], '\0'};
		le->print_handler(line, le->print_handler_ctx);
		i++;
	}

	/* erase everything to the end of current line */
	lineedit_escape_print(le, ESC_ERASE_LINE_END, 0);

	/* restore cursor position */
	lineedit_escape_print(le, ESC_CURSOR_RESTORE, 0);

	return LINEEDIT_BACKSPACE_OK;
}


int32_t lineedit_insert_char(struct lineedit *le, int c) {
	if (u_assert(le != NULL)) {
		return LINEEDIT_INSERT_CHAR_FAILED;
	}

	/* Only printable characters can be inserted. */
	if (c < 32 || c > 127) {
		return LINEEDIT_INSERT_CHAR_FAILED;
	}

	/* we are going to insert 1 character, check if we have enough space */
	if ((le->len - strlen(le->text) - 1) <= 0) {
		return LINEEDIT_INSERT_CHAR_FAILED;;
	}

	int32_t i = strlen(le->text);
	while (i >= (int32_t)le->cursor) {
		le->text[i + 1] = le->text[i];
		i--;
	}

	/* set character at cursor */
	le->text[le->cursor] = c;

	/* and increment cursor */
	le->cursor++;

	/* print character at cursor position */
	char line[2] = {(le->pwchar != 0) ? le->pwchar : c, '\0'};
	le->print_handler(line, le->print_handler_ctx);

	/* save cursor position */
	lineedit_escape_print(le, ESC_CURSOR_SAVE, 0);

	/* now we need to refresh rest of the line */
	i = le->cursor;
	while (le->text[i]) {
		char line[2] = {(le->pwchar != 0) ? le->pwchar : le->text[i], '\0'};
		le->print_handler(line, le->print_handler_ctx);
		i++;
	}

	/* restore cursor position */
	lineedit_escape_print(le, ESC_CURSOR_RESTORE, 0);

	return LINEEDIT_INSERT_CHAR_OK;
}


int32_t lineedit_set_print_handler(struct lineedit *le, int32_t (*print_handler)(const char *line, void *ctx), void *ctx) {
	if (u_assert(le != NULL) ||
	    u_assert(print_handler != NULL)) {
		return LINEEDIT_SET_PRINT_HANDLER_FAILED;
	}

	le->print_handler = print_handler;
	le->print_handler_ctx = ctx;

	return LINEEDIT_SET_PRINT_HANDLER_OK;
}


int32_t lineedit_set_prompt_callback(struct lineedit *le, int32_t (*prompt_callback)(struct lineedit *le, void *ctx), void *ctx) {
	if (u_assert(le != NULL) ||
	    u_assert(prompt_callback != NULL)) {
		return LINEEDIT_SET_PROMPT_CALLBACK_FAILED;
	}

	le->prompt_callback = prompt_callback;
	le->prompt_callback_ctx = ctx;

	return LINEEDIT_SET_PROMPT_CALLBACK_OK;
}


int32_t lineedit_refresh(struct lineedit *le) {
	if (u_assert(le != NULL) ||
	    u_assert(le->print_handler != NULL)) {
		return LINEEDIT_REFRESH_FAILED;
	}

	uint32_t saved = 0;

	/* move cursor to start */
	le->print_handler("\r", le->print_handler_ctx);

	/* erase whole line */
	lineedit_escape_print(le, ESC_ERASE_LINE_END, 0);

	if (le->prompt_callback != NULL) {
		le->prompt_len = le->prompt_callback(le, le->prompt_callback_ctx);
		/* negative number returned, error occured */
		if (le->prompt_len < 0) {
			le->prompt_len = 0;
		}
	}

	uint32_t i = 0;
	while (le->text[i] != '\0') {
		if (le->cursor == i) {
			/* save cursor position */
			lineedit_escape_print(le, ESC_CURSOR_SAVE, 0);
			saved = 1;
		}

		char line[2] = {le->text[i], '\0'};
		le->print_handler(line, le->print_handler_ctx);

		i++;
	}

	/* restore cursor position if needed */
	if (saved) {
		lineedit_escape_print(le, ESC_CURSOR_RESTORE, 0);
	}

	return LINEEDIT_REFRESH_OK;
}


int32_t lineedit_get_cursor(struct lineedit *le, uint32_t *cursor) {
	if (u_assert(le != NULL) ||
	    u_assert(cursor != NULL)) {
		return LINEEDIT_GET_CURSOR_FAILED;
	}

	*cursor = le->cursor;

	return LINEEDIT_GET_CURSOR_OK;
}


/* TODO: can be implemented more effectively. */
int32_t lineedit_set_cursor(struct lineedit *le, uint32_t cursor) {
	if (u_assert(le != NULL)) {
		return LINEEDIT_SET_CURSOR_FAILED;
	}

	if (cursor > strlen(le->text)) {
		return LINEEDIT_SET_CURSOR_FAILED;
	}

	le->cursor = cursor;

	/* move cursor to start */
	le->print_handler("\r", le->print_handler_ctx);

	/* Move cursor to the right up to requested cursor position. */
	for (uint32_t i = 0; i < le->cursor; i++) {
		lineedit_escape_print(le, ESC_CURSOR_RIGHT, 0);
	}

	return LINEEDIT_SET_CURSOR_OK;

}


int32_t lineedit_get_line(struct lineedit *le, char **text) {
	if (u_assert(le != NULL) ||
	    u_assert(text != NULL)) {
		return LINEEDIT_GET_LINE_FAILED;
	}

	*text = le->text;

	return LINEEDIT_GET_LINE_OK;
}


int32_t lineedit_set_line(struct lineedit *le, const char *text) {
	if (u_assert(le != NULL) ||
	    u_assert(text != NULL)) {
		return LINEEDIT_SET_LINE_FAILED;
	}

	strncpy(le->text, text, le->len);
	le->text[le->len - 1] = 0;

	return LINEEDIT_SET_LINE_OK;
}


int32_t lineedit_clear(struct lineedit *le) {
	if (u_assert(le != NULL)) {
		return LINEEDIT_CLEAR_FAILED;
	}

	le->text[0] = '\0';
	le->cursor = 0;

	return LINEEDIT_CLEAR_OK;
}


int32_t lineedit_insert(struct lineedit *le, const char *text) {
	if (u_assert(le != NULL) ||
	    u_assert(text != NULL)) {
		return LINEEDIT_INSERT_FAILED;
	}

	while (*text) {
		lineedit_insert_char(le, *text);
		text++;
	}

	return LINEEDIT_INSERT_OK;
}




