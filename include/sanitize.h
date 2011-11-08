/* ============================================================
* Date  : 2010-2-1
* Author: Brian Hernacki (brian.hernacki@palm.com)
* Copyright 2010 Palm, Inc. All rights reserved.
* ============================================================ */



extern "C" {
	char * sanitizeHtml(const char *input, char **except, bool remove);
	char * sanitizeHtmlUtf8(const char *input, char **except, bool remove);
	char * unsanitizeHtml(char *input);
}
