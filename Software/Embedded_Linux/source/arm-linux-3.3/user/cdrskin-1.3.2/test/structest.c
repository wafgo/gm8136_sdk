#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libburn/libburn.h>

int main(int argc, char **argv)
{
	int i;
	const char *path;
	struct burn_track *track;
	struct burn_disc *disc;
	struct burn_session *session;
	struct burn_source *src;

	burn_initialize();
	burn_msgs_set_severities("NEVER", "ALL", "structest: ");

	disc = burn_disc_create();
	session = burn_session_create();
	burn_disc_add_session(disc, session, BURN_POS_END);

	/* Define a source for all of the tracks */
	path = strdup("/etc/hosts");
	src = burn_file_source_new(path, NULL);

	/* Add ten tracks to a session */
	for (i = 0; i < 10; i++) {
		track = burn_track_create();
		burn_session_add_track(session, track, 0);
		if (burn_track_set_source(track, src) != BURN_SOURCE_OK) {
			printf("problem with the source\n");
			return 0;
		}
	}

	/* Add ten tracks to a session */
	for (i = 0; i < 10; i++) {
		track = burn_track_create();
		burn_session_add_track(session, track, 0);
		if (burn_track_set_source(track, src) != BURN_SOURCE_OK) {
			printf("problem with the source\n");
			return 0;
		}
	}

	/* Delete a session */
	burn_session_remove_track(session, track);

	burn_structure_print_disc(disc);
	return EXIT_SUCCESS;
}
