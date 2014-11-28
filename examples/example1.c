#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "lineedit.h"


/* Global variable holding reference to current command prompt. */
const char *prompt;


/* Output function (print handler) provides a way to output data back to
 * console/terminal */
int32_t output(const char *s, void *ctx) {
	printf("%s", s);
	return 0;
}


/* This callback function is called every time a command prompt output
 * is requested. We are printing global prompt variable to demonstrate
 * this functionality. You can use ctx parameter to get the context in which
 * the callback was called. */
int32_t prompt_callback(struct lineedit *le, void *ctx) {
	/* TODO: nicer API will be provided later */
	lineedit_escape_print(le, ESC_COLOR, LINEEDIT_FG_COLOR_GREEN);
	le->print_handler(prompt, le->print_handler_ctx);
	lineedit_escape_print(le, ESC_DEFAULT, 0);

	return 0;
}

int main(int argc, char *argv[]) {

	struct lineedit line;

	/* Initialize line editor. Return value checking ommited for clarity.
	 * This is the single point where dynamic allocation is used (malloc). */
	lineedit_init(&line, 20);
	lineedit_set_print_handler(&line, output, NULL);
	lineedit_set_prompt_callback(&line, prompt_callback, NULL);

	/* If you want to hide typed characters, set pwchar to nonzero value.
	 * nicer API will be provided later. */
	/* line.pwchar = '*'; */

	/* Set the command prompt. It will be used in the prompt callback function */
	prompt = "prompt > ";

	/* Repeat line editation until "quit" is entered. */
	while (1) {
		lineedit_clear(&line);

		/* Start editation with refreshing current line. Needed to display
		 * command prompt and properly manage cursor positions. It can be used
		 * anytime later also. */
		lineedit_refresh(&line);

		/* Continuously read characters from console input and pass them to
		 * lineedit keypress function. All editing processing is done inside
		 * this single function. Check its return value to see if we are
		 * finished with editing. */
		while (!feof(stdin)) {
			int c = fgetc(stdin);

			int32_t ret = lineedit_keypress(&line, c);

			if (ret == LINEEDIT_ENTER) {
				break;
			}
		}

		/* Get pointer to edited line and print it. */
		char *text;
		lineedit_get_line(&line, &text);
		printf("\nline after editing: '%s'\n", text);

		if (!strcmp(text, "quit")) {
			break;
		}
	}

	/* Do not forget to free the whole thing */
	lineedit_free(&line);
}

