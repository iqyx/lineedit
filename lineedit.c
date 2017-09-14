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


int32_t lineedit_print(struct lineedit *le, const char *s) {
	if (u_assert(le != NULL) ||
	    u_assert(le->print_handler != NULL)) {
		return LINEEDIT_PRINT_FAILED;
	}

	le->print_handler(s, le->print_handler_ctx);

	return LINEEDIT_PRINT_OK;
}


int32_t lineedit_escape_print(struct lineedit *le, enum lineedit_escape_seq esc, int param) {
	if (u_assert(le != NULL)) {
		return LINEEDIT_ESCAPE_PRINT_FAILED;
	}

	char s[20];
	switch (esc) {
		case ESC_CURSOR_LEFT:
			lineedit_print(le, "\x1b[D");
			break;
		case ESC_CURSOR_RIGHT:
			lineedit_print(le, "\x1b[C");
			break;
		case ESC_COLOR:
			snprintf(s, sizeof(s), "\x1b[%dm", param);
			lineedit_print(le, s);
			break;
		case ESC_DEFAULT:
			lineedit_print(le, "\x1b[0m");
			break;
		case ESC_BOLD:
			lineedit_print(le, "\x1b[1m");
			break;
		case ESC_CURSOR_SAVE:
			lineedit_print(le, "\x1b[s");
			break;
		case ESC_CURSOR_RESTORE:
			lineedit_print(le, "\x1b[u");
			break;
		case ESC_ERASE_LINE_END:
			lineedit_print(le, "\x1b[K");
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

	/* Zero the whole structure. */
	memset(le, 0, sizeof(struct lineedit));
	le->len = line_len;
	le->history_size = LINEEDIT_HISTORY_LEN;
	le->escape = ESC_NONE;
	le->recall_index = -1;

	/* Allocate line editing buffer and its corresponding history buffers.
	 * If one of the allocation fails, free any allocated resources and return
	 * with error. */
	le->text = calloc(1, le->len);
	le->history = calloc(1, le->len * le->history_size);
	if (le->text == NULL || le->history == NULL) {
		lineedit_free(le);
		return LINEEDIT_INIT_FAILED;
	}

	/* History is saved in an array of strings le->len long. Initialize the
	 * history by writing 0 at first position of every history entry. */
	for (uint32_t i = 0; i < le->history_size; i++) {
		le->history[i * le->len] = '\0';
	}

	return LINEEDIT_INIT_OK;
}


int32_t lineedit_free(struct lineedit *le) {
	if (u_assert(le != NULL) ||
	    u_assert(le->text != NULL)) {
		return LINEEDIT_FREE_FAILED;
	}

	free(le->history);
	free(le->text);

	return LINEEDIT_FREE_OK;
}


int32_t lineedit_history_append(struct lineedit *le, const char *line) {
	if (u_assert(le != NULL) ||
	    u_assert(line != NULL)) {
		return LINEEDIT_HISTORY_APPEND_FAILED;
	}

	/* Shift all history entries first. The newest one has index 0. */
	for (uint32_t i = le->history_size - 1; i > 0 ; i--) {
		strlcpy(le->history + (i * le->len), le->history + ((i - 1) * le->len), le->len);
	}
	strlcpy(le->history, line, le->len);

	return LINEEDIT_HISTORY_APPEND_OK;
}


int32_t lineedit_history_recall(struct lineedit *le, char **line, int32_t recall_index) {
	if (u_assert(le != NULL) ||
	    u_assert(line != NULL)) {
		return LINEEDIT_HISTORY_RECALL_FAILED;
	}

	if ((recall_index >= (int32_t)le->history_size) || (recall_index < -1)) {
		return LINEEDIT_HISTORY_RECALL_FAILED;
	}

	if (recall_index == -1) {
		*line = "";
	} else {
		*line = le->history + (recall_index * le->len);
	}
	return LINEEDIT_HISTORY_RECALL_OK;
}


int32_t lineedit_keypress(struct lineedit *le, int c) {
	if (u_assert(le != NULL)) {
		return LINEEDIT_FAILED;
	}

	if (le->escape == ESC_NONE) {

		switch (c) {
			/* check for TAB */
			case 0x09:
				return LINEEDIT_TAB;

			/* check for line feed */
			case 0x0a:
			case 0x0b:
			case 0x0c:
			case 0x0d:
				/* save current line to the history and reset recall
				 * index to point to the current line (-1) */
				lineedit_history_append(le, le->text);
				le->recall_index = -1;
				return LINEEDIT_ENTER;

			case 0x12:
				lineedit_refresh(le);
				break;

			/* interrupt escape sequence */
			case 0x18:
			case 0x1a:
				le->escape = ESC_NONE;
				break;

			/* check for ESC */
			case 0x1b:
				le->escape = ESC_ESC;
				break;

			/* check for DEL (backspace) */
			case 0x7f:
				/* Do not check return value, if we are unable to do backspace,
				 * we just ignore it. */
				lineedit_backspace(le);
				break;

			/* check for CSI */
			case 0x9b:
				le->escape = ESC_CSI;
				le->csi_escape_mod = 0;
				break;

			default:
				/* other alphanumeric characters */
				if (c >= 32 && c <= 126) {
					/* Do not check return value, if we are unable to insert it,
					 * we just ignore the character. */
					lineedit_insert_char(le, c);
				}
				break;
		}

	} else if (le->escape == ESC_ESC) {

		/* if ESC is set and '[' character was received, start CSI sequence */
		if (c == '[') {
			le->escape = ESC_CSI;
			le->csi_escape_mod = 0;
		}

		/* if ESC is set and ']' character was received, start OSC sequence */
		if (c == ']') {
			le->escape = ESC_OSC;
		}

	} else if (le->escape == ESC_CSI) {

		/* if CSI is set, try to read first alphanumeric character (parameters are ignored) */
		switch (c) {
			/* Escape modifier, we continue with CSI escape flag set. */
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				le->csi_escape_mod = le->csi_escape_mod * 10 + (c - '0');
				break;

			case 'A': {
				/* Move cursor up (previous history entry). */
				char *hist_command;
				if (lineedit_history_recall(le, &hist_command, le->recall_index + 1) == LINEEDIT_HISTORY_RECALL_OK) {
					lineedit_set_line(le, hist_command);
					lineedit_refresh(le);
					le->recall_index++;
				}
				break;
			}

			case 'B': {
				/* Move cursor down (next history entry). */
				char *hist_command;
				if (lineedit_history_recall(le, &hist_command, le->recall_index - 1) == LINEEDIT_HISTORY_RECALL_OK) {
					lineedit_set_line(le, hist_command);
					lineedit_refresh(le);
					le->recall_index--;
				}
				break;
			}

			case 'C':
				/* move cursor right */
				if (le->cursor < (strlen(le->text))) {
					le->cursor++;
					lineedit_escape_print(le, ESC_CURSOR_RIGHT, 1);
				}
				break;

			case 'D':
				/* move cursor left */
				if (le->cursor > 0) {
					le->cursor--;
					lineedit_escape_print(le, ESC_CURSOR_LEFT, 1);
				}
				break;

			case '~':
				/* Delete key. */
				lineedit_backspace(le);
				break;

			default:
				break;

		}

		le->escape = ESC_NONE;

	} else if (le->escape == ESC_OSC) {

		le->escape = ESC_NONE;
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
		lineedit_print(le, line);
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
	lineedit_print(le, line);

	/* save cursor position */
	lineedit_escape_print(le, ESC_CURSOR_SAVE, 0);

	/* now we need to refresh rest of the line */
	i = le->cursor;
	while (le->text[i]) {
		char line[2] = {(le->pwchar != 0) ? le->pwchar : le->text[i], '\0'};
		lineedit_print(le, line);
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
	if (u_assert(le != NULL)) {
		return LINEEDIT_REFRESH_FAILED;
	}

	uint32_t saved = 0;

	/* move cursor to start */
	lineedit_print(le, "\r");

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
		lineedit_print(le, line);

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
	lineedit_print(le, "\r");

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
	le->cursor = strlen(le->text);

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




