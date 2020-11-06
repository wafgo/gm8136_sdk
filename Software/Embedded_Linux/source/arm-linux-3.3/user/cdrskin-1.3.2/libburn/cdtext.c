
/* Copyright (c) 2011 Thomas Schmitt <scdbackup@gmx.net>
   Provided under GPL version 2 or later.
*/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "libburn.h"
#include "init.h"
#include "structure.h"
#include "options.h"
#include "util.h"

#include "libdax_msgs.h"
extern struct libdax_msgs *libdax_messenger;


/* --------------------- Production of CD-TEXT packs -------------------- */


struct burn_pack_cursor {
	unsigned char *packs;
	int num_packs;
	int td_used;
	int hiseq[8];
	int pack_count[16];
	int track_offset;
};


/* @param flag bit0= double_byte characters
*/
int burn_create_new_pack(int pack_type, int track_no, int double_byte,
				int block, int char_pos,
				struct burn_pack_cursor *crs, int flag)
{
	int idx;

	if (crs->num_packs >= Libburn_leadin_cdtext_packs_maX) {
		libdax_msgs_submit(libdax_messenger, -1, 0x0002018b,
				LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
				"Too many CD-TEXT packs", 0, 0);
		return 0;
	}
	if (crs->hiseq[block] >= 255) {
		libdax_msgs_submit(libdax_messenger, -1, 0x0002018e,
				LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
				"Too many CD-TEXT packs in block", 0, 0);
		return 0;
	}
	if (char_pos > 15)
		char_pos = 15;
	else if (char_pos < 0)
		char_pos = 0;
	idx = crs->num_packs * 18;
	crs->packs[idx++] = pack_type;
	crs->packs[idx++] = track_no;
	crs->packs[idx++] = crs->hiseq[block];
	crs->packs[idx++] = ((flag & 1) << 7) | (block << 4) | char_pos;
	crs->hiseq[block]++;
	crs->td_used = 0;
	crs->pack_count[pack_type - Libburn_pack_type_basE]++;
	return 1;
}


/* Plain implementation of polynomial division on a Galois field, where
   addition and subtraction both are binary exor. Euclidian algorithm.
   Divisor is x^16 + x^12 + x^5 + 1 = 0x11021.
*/
static int crc_11021(unsigned char *data, int count, int flag)
{
	int acc = 0, i;

	for (i = 0; i < count * 8 + 16; i++) {
		acc = (acc << 1);
		if (i < count * 8)
			acc |= ((data[i / 8] >> (7 - (i % 8))) & 1);
		if (acc & 0x10000)
			acc ^= 0x11021;
	}
	return acc;
}


/* @param flag bit0= repair mismatching checksums
               bit1= repair checksums if all pack CRCs are 0
   @return 0= no mismatch , >0 number of unrepaired mismatches
                            <0 number of repaired mismatches that were not 0
*/
int burn_cdtext_crc_mismatches(unsigned char *packs, int num_packs, int flag)
{
	int i, residue, count = 0, repair;
	unsigned char crc[2];

	repair = flag & 1;
	if (flag & 2) {
		for (i = 0; i < num_packs * 18; i += 18)
			if (packs[i + 16] || packs[i + 17])
		break;
		if (i == num_packs * 18)
			repair = 1;
	}
	for (i = 0; i < num_packs * 18; i += 18) {
		residue = crc_11021(packs + i, 16, 0);
		crc[0] = ((residue >> 8) & 0xff) ^ 0xff;
		crc[1] = ((residue     ) & 0xff) ^ 0xff;
		if(crc[0] != packs[i + 16] || crc[1] != packs[i + 17]) {
			if (repair) {
				if (packs[i + 16] || packs[i + 17])
					count--;
				packs[i + 16] = crc[0];
				packs[i + 17] = crc[1];
			} else
				count++;
		}
		
	}
	return count;
}


static int burn_finalize_text_pack(struct burn_pack_cursor *crs, int flag)
{
	int residue = 0, i, idx;

	idx = 18 * crs->num_packs;
	for(i = 4 + crs->td_used; i < 16; i++)
		crs->packs[idx + i] = 0;
	crs->td_used = 12;

	/* MMC-3 Annex J : CRC Field consists of 2 bytes.
	   The polynomial is X16 + X12 + X5 + 1. All bits shall be inverted.
	*/
	residue = crc_11021(crs->packs + idx, 16, 0) ^ 0xffff;

	crs->packs[idx + 16] = (residue >> 8) & 0xff;
	crs->packs[idx + 17] = residue & 0xff;
	crs->num_packs++;
	crs->td_used = 0;
	return 1;
}


/* @param flag bit0= double_byte characters
*/
static int burn_create_tybl_packs(unsigned char *payload, int length,
				int track_no, int pack_type, int block,
				struct burn_pack_cursor *crs, int flag)
{
	int i, ret, binary_part = 0, char_pos;

	if (pack_type == 0x87)
		binary_part = 2;
	else if ((pack_type >= 0x88 && pack_type <= 0x8c) || pack_type == 0x8f)
		binary_part = length;
	for(i = 0; i < length; i++) {
		if (crs->td_used == 0 || crs->td_used >= 12) {
			if (crs->td_used > 0) {
				ret = burn_finalize_text_pack(crs, 0);
				if (ret <= 0)
					return ret;
			}
			char_pos = (i - binary_part) / (1 + (flag & 1));
			ret = burn_create_new_pack(pack_type, track_no,
					(flag & 1), block, char_pos,
					crs, flag & 1);
			if (ret <= 0)
				return ret;
		}
		crs->packs[crs->num_packs * 18 + 4 + crs->td_used] =
								payload[i];
		crs->td_used++;
	}
	return 1;
}


/* Finalize block by 0x8f. Set bytes 20 to 27 to 0 for now. */
static int burn_create_bl_size_packs(int block, unsigned char *char_codes,
					unsigned char *copyrights,
					unsigned char *languages,
					int num_tracks,
					struct burn_pack_cursor *crs, int flag)
{
	int i, ret;
	unsigned char payload[12];

	payload[0] = char_codes[block];
	payload[1] = crs->track_offset;
	payload[2] = num_tracks + crs->track_offset - 1;
	payload[3] = copyrights[block];
	for (i = 0; i < 8; i++)
		payload[i + 4] = crs->pack_count[i];
	ret = burn_create_tybl_packs(payload, 12, 0, 0x8f, block, crs, 0);
	if (ret <= 0)
		return ret;

	for (i = 0; i < 7; i++)
		payload[i] = crs->pack_count[i + 8];
	payload[7] = 3; /* always 3 packs of type 0x8f */
	for (i = 0; i < 4; i++) {
		/* Will be set when all blocks are done */
		payload[i + 8] = 0;
	}
	ret = burn_create_tybl_packs(payload, 12, 1, 0x8f, block, crs, 0);
	if (ret <= 0)
		return ret;

	for (i = 0; i < 4; i++) {
		/* Will be set when all blocks are done */
		payload[i] = 0;
	}
	for (i = 0; i < 8; i++) {
		payload[i + 4] = languages[i];
	}
	ret = burn_create_tybl_packs(payload, 12, 2, 0x8f, block, crs, 0);
	if (ret <= 0)
		return ret;
	ret = burn_finalize_text_pack(crs, 0);
	if (ret <= 0)
		return ret;

	for (i = 0; i < 16; i++)
		crs->pack_count[i] = 0;
	return 1;
}


/* Text packs of track for type and block
   @param flag bit0= write TAB, because content is identical to previous track
*/
static int burn_create_tybl_t_packs(struct burn_track *t, int track_no,
					int pack_type, int block,
					struct burn_pack_cursor *crs, int flag)
{
	int ret, length = 0, idx, double_byte, flags= 0;
	unsigned char *payload = NULL, dummy[8];
	struct burn_cdtext *cdt;

	cdt = t->cdtext[block];
	idx = pack_type - Libburn_pack_type_basE;
	if (cdt != NULL) {
		if (cdt->length[idx] > 0) {
			payload = cdt->payload[idx];
			length = cdt->length[idx];
		}
		flags = cdt->flags;
	}
	if (payload == NULL) {
		dummy[0]= 0;
		payload = dummy;
		length = strlen((char *) dummy) + 1;
	}
	double_byte = !!(flags & (1 <<(pack_type - Libburn_pack_type_basE)));
	if (flag & 1) {
		length = 0;
		dummy[length++] = 9;
		if (double_byte)
			dummy[length++] = 9;
		dummy[length++] = 0;
		if (double_byte)
			dummy[length++] = 0;
		payload = dummy;
	}
	ret = burn_create_tybl_packs(payload, length, track_no,
					pack_type, block, crs, double_byte);
	return ret;
}


/* Check whether the content is the same as in the previous pack. If so,
   advise to use the TAB abbreviation.
*/
static int burn_decide_cdtext_tab(int block, int pack_type,
				  struct burn_cdtext *cdt_curr,
			          struct burn_cdtext *cdt_prev, int flag)
{
	int length, j, idx;

	idx = pack_type - Libburn_pack_type_basE;
	if (cdt_curr == NULL || cdt_prev == NULL)
		return 0;
	if (((cdt_curr->flags >> idx) & 1) != ((cdt_prev->flags >> idx) & 1))
		return 0;
	length = cdt_curr->length[idx];
	if (length != cdt_prev->length[idx] ||
				length <= 1 + ((cdt_curr->flags >> idx) & 1))
		return 0;
	for (j = 0; j < length; j++)
		if (cdt_curr->payload[idx][j] != cdt_prev->payload[idx][j])
	break;
	if (j < length)
		return 0;
	return 1;
}


/* Text packs of session and of tracks (if applicable), for type and block
*/
static int burn_create_tybl_s_packs(struct burn_session *s,
					int pack_type, int block,
					struct burn_pack_cursor *crs, int flag)
{
	int i, ret, idx, double_byte, use_tab;
	struct burn_cdtext *cdt;

	cdt = s->cdtext[block];
	idx = pack_type - Libburn_pack_type_basE;
	if (cdt->length[idx] == 0 || cdt->payload[idx] == NULL)
		return 1;

	double_byte = !!(cdt->flags &
			 (1 <<(pack_type - Libburn_pack_type_basE)));
	ret = burn_create_tybl_packs(cdt->payload[idx], cdt->length[idx], 0,
					pack_type, block, crs, double_byte);
	if (ret <= 0)
		return ret;

	if ((pack_type < 0x80 || pack_type > 0x85) && pack_type != 0x8e) {
		ret = burn_finalize_text_pack(crs, 0);
		return ret;
	}

	for (i = 0; i < s->tracks; i++) {
		if (i > 0)
			use_tab = burn_decide_cdtext_tab(block, pack_type,
					  s->track[i]->cdtext[block],
				          s->track[i - 1]->cdtext[block], 0);
		else
			use_tab = 0;
		ret = burn_create_tybl_t_packs(s->track[i],
					i + crs->track_offset, pack_type,
					block, crs, use_tab);
		if (ret <= 0)
			return ret;
	}
	/* Fill up last pack with 0s */
	ret = burn_finalize_text_pack(crs, 0);
	return ret;
}


/* ts B11210 : API */
/* @param flag bit0= do not return CD-TEXT packs, but return number of packs
*/
int burn_cdtext_from_session(struct burn_session *s,
				unsigned char **text_packs, int *num_packs,
				int flag)
{
	int pack_type, block, ret, i, idx, j, residue;
	struct burn_pack_cursor crs;

	if (text_packs == NULL || num_packs == NULL) {
		flag |= 1;
	} else if (!(flag & 1)) {
		*text_packs = NULL;
		*num_packs = 0;
	}
	memset(&crs, 0, sizeof(struct burn_pack_cursor));
	crs.track_offset = s->firsttrack;
	BURN_ALLOC_MEM(crs.packs, unsigned char,
					Libburn_leadin_cdtext_packs_maX * 18);

	for (block = 0; block < 8; block++)
		if (s->cdtext[block] != NULL)
	break;
	if (block == 8)
		{ret = 1; goto ex;}

	for (block= 0; block < 8; block++) {
		if (s->cdtext[block] == NULL)
	continue;
		for (pack_type = 0x80;
		     pack_type < 0x80 + Libburn_pack_num_typeS; pack_type++) {
			if (pack_type == 0x8f)
		continue;
			ret = burn_create_tybl_s_packs(s,
						pack_type, block, &crs, 0);
			if (ret <= 0)
				goto ex;
		}
		ret = burn_create_bl_size_packs(block,
				s->cdtext_char_code, s->cdtext_copyright,
				s->cdtext_language, s->tracks, &crs, 0);
		if (ret <= 0)
			goto ex;
	}

	/* Insert the highest sequence numbers of each block into
	   the 0x8f packs 2 and 3 (bytes 20 to 27)
	*/
	for (i = 0; i < crs.num_packs; i++) {
		idx = i * 18;
		if (crs.packs[idx] == 0x8f && crs.packs[idx + 1] == 1) {
			for (j = 0; j < 4; j++)
				if (crs.hiseq[j] > 0)
					crs.packs[idx + 4 + 8 + j] =
							crs.hiseq[j] - 1;
				else
					crs.packs[idx + 4 + 8 + j] = 0;
		} else if (crs.packs[idx] == 0x8f && crs.packs[idx + 1] == 2) {
			for (j = 0; j < 4; j++)
				if (crs.hiseq[j + 4] > 0)
					crs.packs[idx + 4 + j] =
							crs.hiseq[j + 4] - 1;
				else
					crs.packs[idx + 4 + j] = 0;
		} else
	continue;
		/* Re-compute checksum */
		residue = crc_11021(crs.packs + idx, 16, 0) ^ 0xffff;
		crs.packs[idx + 16] = (residue >> 8) & 0xff;
		crs.packs[idx + 17] = residue & 0xff;
	}

	ret = 1;
ex:;
	if (ret <= 0 || (flag & 1)) {
		if (ret > 0)
			ret = crs.num_packs;
		else if (flag & 1)
			ret = -1;
		BURN_FREE_MEM(crs.packs);
	} else {
		if (crs.num_packs > 0)
			*text_packs = crs.packs;
		else
			BURN_FREE_MEM(crs.packs);
		*num_packs = crs.num_packs;
	}
	return(ret);
}


/* ---------------- Reader of Sony Input Sheet Version 0.7T ------------- */


/* @param flag bit0= allow two byte codes 0xNNNN or 0xNN 0xNN
*/
static int v07t_hexcode(char *payload, int flag)
{
	unsigned int x;
	int lo, hi, l;
	char buf[10], *cpt;

	l = strlen(payload);
	if (strncmp(payload, "0x", 2) != 0)
		return -1;
	if ((l == 6 || l == 9) && (flag & 1))
		goto double_byte;
	if (strlen(payload) != 4)
		return -1;
	if (!(isxdigit(payload[2]) && isxdigit(payload[3])))
		return -1;
	sscanf(payload + 2, "%x", &x);
	return x;

double_byte:;
	strcpy(buf, payload);
	buf[4] = 0;
	hi = v07t_hexcode(buf, 0);
	if (strlen(payload) == 6) {
		buf[4] = payload[4];
		buf[2] = '0';
		buf[3] = 'x';
		cpt = buf + 2;
	} else {
		if(payload[4] != 32 && payload[4] != 9)
			return(-1);
		cpt = buf + 5;
	}
	lo = v07t_hexcode(cpt, 0);
	if (lo < 0 || hi < 0)
		return -1;
	return ((hi << 8) | lo);
}


static int v07t_cdtext_char_code(char *payload, int flag)
{
	int ret;
	char *msg = NULL;

	ret = v07t_hexcode(payload, 0);
	if (ret >= 0)
		return ret;
	if (strstr(payload, "8859") != NULL)
		return 0x00;
	else if(strstr(payload, "ASCII") != NULL)
		return 0x01;
	else if(strstr(payload, "JIS") != NULL)
		return 0x80;

	BURN_ALLOC_MEM(msg, char, 160);
	sprintf(msg, "Unknown v07t Text Code '%.80s'", payload);
	libdax_msgs_submit(libdax_messenger, -1, 0x00020191,
				LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
				burn_printify(msg), 0, 0);
	ret = -1;
ex:;
	BURN_FREE_MEM(msg);
	return ret;
}


static int v07t_cdtext_lang_code(char *payload, int flag)
{
	int i, ret;
	static char *languages[128] = {
		BURN_CDTEXT_LANGUAGES_0X00,
		BURN_CDTEXT_FILLER,
		BURN_CDTEXT_LANGUAGES_0X45
	};
	char *msg = NULL;

	ret = v07t_hexcode(payload, 0);
	if (ret >= 0)
		return ret;
	if (payload[0] != 0)
		for(i = 0; i < 128; i++)
			if(strcmp(languages[i], payload) == 0)
				return i;

	BURN_ALLOC_MEM(msg, char, 160);
	sprintf(msg, "Unknown v07t Language Code '%.80s'", payload);
	libdax_msgs_submit(libdax_messenger, -1, 0x00020191,
				LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
				burn_printify(msg), 0, 0);
	ret = -1;
ex:;
	BURN_FREE_MEM(msg);
	return ret;
}


static int v07t_cdtext_genre_code(char *payload, int flag)
{
	int i, ret;
	static char *genres[BURN_CDTEXT_NUM_GENRES] = {
		BURN_CDTEXT_GENRE_LIST
	};
	char *msg = NULL;
	
	ret = v07t_hexcode(payload, 1);
	if(ret >= 0)
		return ret;
	for (i= 0; i < BURN_CDTEXT_NUM_GENRES; i++)
		if (strcmp(genres[i], payload) == 0)
			return i;

	BURN_ALLOC_MEM(msg, char, 160);
	sprintf(msg, "Unknown v07t Genre Code '%.80s'", payload);
	libdax_msgs_submit(libdax_messenger, -1, 0x00020191,
				LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
				burn_printify(msg), 0, 0);
	ret = -1;
ex:;
	BURN_FREE_MEM(msg);
	return ret;
}


static int v07t_cdtext_len_db(char *payload, int *char_code,
				int *length, int *double_byte, int flag)
{
	if (*char_code < 0)
		*char_code = 0x00;
	*double_byte = (*char_code == 0x80);
	*length = strlen(payload) + 1 + *double_byte;
	return 1;
}


static int v07t_cdtext_to_session(struct burn_session *session, int block,
				char *payload, int *char_code, int pack_type,
				char *pack_type_name, int flag)
{
	int length, double_byte, ret;

	ret = v07t_cdtext_len_db(payload, char_code, &length, &double_byte, 0);
	if (ret <= 0)
		return ret;
	ret = burn_session_set_cdtext(session, block, pack_type,
			pack_type_name, (unsigned char *) payload, length,
			double_byte);
	return ret;
}


static int v07t_cdtext_to_track(struct burn_track *track, int block,
			char *payload, int *char_code, int pack_type,
			char *pack_type_name, int flag)
{
	int length, double_byte, ret;

	ret = v07t_cdtext_len_db(payload, char_code, &length, &double_byte, 0);
	if (ret <= 0)
		return ret;
	ret = burn_track_set_cdtext(track, block, pack_type, pack_type_name,
			(unsigned char *) payload, length, double_byte);
	return ret;
}


static int v07t_apply_to_session(struct burn_session *session, int block,
                     int char_codes[8], int copyrights[8], int languages[8],
                     int session_attr_seen[16], int track_attr_seen[16],
                     int genre_code, char *genre_text, int flag)
{
	int i, ret, length;
	char *line = NULL;

	BURN_ALLOC_MEM(line, char, 4096);

	for (i= 0x80; i <= 0x8e; i++) {
		if (i > 0x85 && i != 0x8e)
	continue;
		if (session_attr_seen[i - 0x80] || !track_attr_seen[i - 0x80])
	continue;
		ret = v07t_cdtext_to_session(session, block, "",
					char_codes + block, i, NULL, 0);
		if (ret <= 0)
			goto ex;
	}
	if (genre_code >= 0 && genre_text[0]) {
		line[0] = (genre_code >> 8) & 0xff;
		line[1] = genre_code & 0xff;
		strcpy(line + 2, genre_text);
		length = 2 + strlen(line + 2) + 1;
		ret = burn_session_set_cdtext(session, block, 0, "GENRE",
					(unsigned char *) line, length, 0);
		if (ret <= 0)
			goto ex;
	}
	ret = burn_session_set_cdtext_par(session, char_codes, copyrights,
								languages, 0);
	if (ret <= 0)
		goto ex;
	for (i = 0; i < 8; i++)
		char_codes[i] = copyrights[i] = languages[i]= -1;
	for (i = 0; i < 16; i++)
		session_attr_seen[i] = track_attr_seen[i] = 0;
	genre_text[0] = 0;
	ret = 1;
ex:
	BURN_FREE_MEM(line);
	return ret;
}


/* ts B11215 API */
/* @param flag bit0= permission to read multiple blocks from the same sheet
               bit1= do not attach CATALOG to session or ISRC to track for
                     writing to Q sub-channel
*/
int burn_session_input_sheet_v07t(struct burn_session *session,
					char *path, int block, int flag)
{
	int ret = 0, num_tracks, char_codes[8], copyrights[8], languages[8], i;
	int genre_code = -1, track_offset = 1, pack_type, tno, tnum;
	int session_attr_seen[16], track_attr_seen[16];
	int int0x00 = 0x00, int0x01 = 0x01;
	int additional_blocks = -1, line_count = 0, enable_multi_block = 0;
	struct stat stbuf;
	FILE *fp = NULL;
	char *line = NULL, *eq_pos, *payload, *genre_text = NULL, track_txt[3];
	char *msg = NULL;
	struct burn_track **tracks;

	BURN_ALLOC_MEM(msg, char, 4096);
	BURN_ALLOC_MEM(line, char, 4096);
	BURN_ALLOC_MEM(genre_text, char, 160);

	for (i = 0; i < 8; i++)
		char_codes[i] = copyrights[i] = languages[i]= -1;
	for (i = 0; i < 16; i++)
		session_attr_seen[i] = track_attr_seen[i] = 0;
	genre_text[0] = 0;

	tracks = burn_session_get_tracks(session, &num_tracks);
	if (stat(path, &stbuf) == -1) {
cannot_open:;
		sprintf(msg, "Cannot open CD-TEXT input sheet v07t '%.4000s'",
			path);
		libdax_msgs_submit(libdax_messenger, -1, 0x00020193,
				LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
				burn_printify(msg), errno, 0);
		ret = 0; goto ex;
	}
	if (!S_ISREG(stbuf.st_mode)) {
		sprintf(msg,
		  "File is not of usable type: CD-TEXT input sheet v07t '%s'",
			path);
		libdax_msgs_submit(libdax_messenger, -1, 0x00020193,
				LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
				burn_printify(msg), 0, 0);
		ret = 0; goto ex;
	}

	fp = fopen(path, "rb");
	if (fp == NULL)
		goto cannot_open;

	while (1) {
		if (burn_sfile_fgets(line, 4095, fp) == NULL) {
			if (!ferror(fp))
	break;
			sprintf(msg,
	"Cannot read all bytes from  CD-TEXT input sheet v07t '%.4000s'",
				path);
			libdax_msgs_submit(libdax_messenger, -1, 0x00020193,
				LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
				burn_printify(msg), 0, 0);
			ret = 0; goto ex;
		}
		line_count++;
		if (strlen(line) == 0)
	continue;
		eq_pos = strchr(line, '=');
		if (eq_pos == NULL) {
			sprintf(msg,
		"CD-TEXT v07t input sheet line without '=' : '%.4000s'",
				line);
			libdax_msgs_submit(libdax_messenger, -1, 0x00020194,
				LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
				burn_printify(msg), 0, 0);
			ret = 0; goto ex;
		}
		for (payload = eq_pos + 1; *payload == 32 || *payload == 9;
		     payload++);
		*eq_pos = 0;
		for (eq_pos--;
		     (*eq_pos == 32 || *eq_pos == 9) && eq_pos > line;
		     eq_pos--)
			*eq_pos= 0;

		if (payload[0] == 0)
	continue;

		if (strcmp(line, "Text Code") == 0) {
			ret = v07t_cdtext_char_code(payload, 0);
			if (ret < 0)
				goto ex;
			if (char_codes[block] >= 0 &&
			    char_codes[block] != ret) {
				libdax_msgs_submit(libdax_messenger, -1,
					0x00020192, LIBDAX_MSGS_SEV_FAILURE,
					LIBDAX_MSGS_PRIO_HIGH,
					"Unexpected v07t Text Code change",
					0, 0);
				ret = 0; goto ex;
			}
			char_codes[block] = ret;

		} else if (strcmp(line, "Language Code") == 0) {
			ret = v07t_cdtext_lang_code(payload, 0);
			if(ret < 0)
				goto ex;
			languages[block] = ret;

		} else if (strcmp(line, "0x80") == 0 ||
				strcmp(line, "Album Title") == 0) {
			ret = v07t_cdtext_to_session(session, block, payload,
					char_codes + block, 0, "TITLE", 0);
			if (ret <= 0)
				goto ex;
			session_attr_seen[0x0] = 1;

		} else if (strcmp(line, "0x81") == 0 ||
				strcmp(line, "Artist Name") == 0) {
			ret = v07t_cdtext_to_session(session, block, payload,
					char_codes + block, 0, "PERFORMER", 0);
			if (ret <= 0)
				goto ex;
			session_attr_seen[0x1] = 1;

		} else if (strcmp(line, "0x82") == 0 ||
				strcmp(line, "Songwriter") == 0) {
			ret = v07t_cdtext_to_session(session, block, payload,
					char_codes + block, 0, "SONGWRITER",
					0);
			if (ret <= 0)
				goto ex;
			session_attr_seen[0x2] = 1;

		} else if (strcmp(line, "0x83") == 0 ||
				strcmp(line, "Composer") == 0) {
			ret = v07t_cdtext_to_session(session, block, payload,
					char_codes + block, 0, "COMPOSER", 0);
			if (ret <= 0)
				goto ex;
			session_attr_seen[0x3] = 1;

		} else if (strcmp(line, "0x84") == 0 ||
				 strcmp(line, "Arranger") == 0) {
			ret = v07t_cdtext_to_session(session, block, payload,
					char_codes + block, 0, "ARRANGER", 0);
			if (ret <= 0)
				goto ex;
			session_attr_seen[0x4] = 1;

		} else if (strcmp(line, "0x85") == 0 ||
				strcmp(line, "Album Message") == 0) {
			ret = v07t_cdtext_to_session(session, block, payload,
					char_codes + block, 0, "MESSAGE", 0);
			if (ret <= 0)
				goto ex;
			session_attr_seen[0x5] = 1;

		} else if (strcmp(line, "0x86") == 0 ||
				      strcmp(line, "Catalog Number") == 0) {
			ret = v07t_cdtext_to_session(session, block, payload,
					&int0x01, 0, "DISCID", 0);
			if(ret <= 0)
				goto ex;

		} else if (strcmp(line, "Genre Code") == 0) {
			genre_code = v07t_cdtext_genre_code(payload, 0);
			if (genre_code < 0) {
				ret = 0; goto ex;
			}

		} else if (strcmp(line, "Genre Information") == 0) {
			strncpy(genre_text, payload, 159);
			genre_text[159] = 0;

		} else if (strcmp(line, "0x8d") == 0 ||
				strcmp(line, "Closed Information") == 0) {
			ret = v07t_cdtext_to_session(session, block, payload,
					&int0x00, 0, "CLOSED", 0);
			if (ret <= 0)
				goto ex;

		} else if(strcmp(line, "0x8e") == 0 ||
				strcmp(line, "UPC / EAN") == 0) {
			ret = v07t_cdtext_to_session(session, block, payload,
					&int0x01, 0, "UPC_ISRC", 0);
			if (ret <= 0)
				goto ex;
			if (!(flag & 2)) {
				memcpy(session->mediacatalog, payload, 13);
				session->mediacatalog[13] = 0;
			}
			session_attr_seen[0xe] = 1;

		} else if (strncmp(line, "Disc Information ", 17) == 0) {

			/* >>> ??? is this good for anything ? */;

		} else if (strcmp(line, "Input Sheet Version") == 0) {
			if (strcmp(payload, "0.7T") != 0) {
				sprintf(msg,
		"Wrong Input Sheet Version '%.4000s'. Expected '0.7T'.",
					payload);
				libdax_msgs_submit(libdax_messenger, -1,
					0x00020194, LIBDAX_MSGS_SEV_FAILURE,
					LIBDAX_MSGS_PRIO_HIGH,
					burn_printify(msg), 0, 0);
				ret = 0; goto ex;
			}
			if (flag & 1)
				if (line_count == 1)
					enable_multi_block = 1;
			if (enable_multi_block) {
				if (additional_blocks >= 0) {
					if (block == 7) {
						libdax_msgs_submit(
				libdax_messenger, -1, 0x000201a0,
				LIBDAX_MSGS_SEV_WARNING, LIBDAX_MSGS_PRIO_HIGH,
				"Maximum number of CD-TEXT blocks exceeded",
						0, 0);
	break;
					}
					ret = v07t_apply_to_session(
						session, block, char_codes,
						copyrights, languages,
						session_attr_seen,
						track_attr_seen,
						genre_code, genre_text, 0);
					if (ret <= 0)
						goto ex;
					block++;
				}
				additional_blocks++;
			}

		} else if (strcmp(line, "Remarks") == 0) {
			;

		} else if (strcmp(line, "Text Data Copy Protection") == 0) {
			ret = v07t_hexcode(payload, 0);
			if (ret >= 0)
				copyrights[block] = ret;
			else if (strcmp(payload, "ON") == 0)
				copyrights[block] = 0x03;
			else if (strcmp(payload, "OFF") == 0)
				copyrights[block] = 0x00;
			else {
				sprintf(msg,
			   "Unknown v07t Text Data Copy Protection '%.4000s'",
				payload);
				libdax_msgs_submit(libdax_messenger, -1,
					0x00020191, LIBDAX_MSGS_SEV_FAILURE,
					LIBDAX_MSGS_PRIO_HIGH,
					burn_printify(msg), 0, 0);

				ret = 0; goto ex;
			}

		} else if (strcmp(line, "First Track Number") == 0) {
			ret = -1;
			sscanf(payload, "%d", &ret);
			if (ret <= 0 || ret > 99) {
bad_tno:;
				sprintf(msg,
	 		"Inappropriate v07t First Track Number '%.4000s'",
					payload);
				libdax_msgs_submit(libdax_messenger, -1,
					0x00020194, LIBDAX_MSGS_SEV_FAILURE,
					LIBDAX_MSGS_PRIO_HIGH,
					burn_printify(msg), 0, 0);
				ret = 0; goto ex;
			} else {
				track_offset = ret;
				ret = burn_session_set_start_tno(session,
							track_offset, 0);
				if (ret <= 0)
					goto ex;
			}

		} else if (strcmp(line, "Last Track Number") == 0) {
			ret = -1;
			sscanf(payload, "%d", &ret);
			if (ret < 0) {
				goto bad_tno;
			} else {

				/* >>> ??? Is it good for anything ? */;

			}

		} else if (strncmp(line, "Track ", 6) == 0) {
			tno = -1;
			sscanf(line + 6, "%d", &tno);
			if (tno < 1 || tno - track_offset < 0 ||
				tno - track_offset >= num_tracks) {
				track_txt[0] = line[6];
				track_txt[1] = line[7];
				track_txt[2] = 0;
bad_track_no:;
				sprintf(msg,
				   "Inappropriate v07t Track number '%.3900s'",
					track_txt);
				sprintf(msg + strlen(msg),
				    "  (acceptable range: %2.2d to %2.2d)",
				    track_offset,
				    num_tracks + track_offset - 1);
				libdax_msgs_submit(libdax_messenger, -1,
					  0x00020194, LIBDAX_MSGS_SEV_FAILURE,
					  LIBDAX_MSGS_PRIO_HIGH,
					  burn_printify(msg), 0, 0);
				ret = 0; goto ex;
			}
			tnum = tno - track_offset;

			if (strcmp(line, "0x80") == 0 ||
			    strcmp(line + 9, "Title") == 0)
				pack_type = 0x80;
			else if (strcmp(line + 9, "0x81") == 0 ||
					strcmp(line + 9, "Artist") == 0)
				pack_type = 0x81;
			else if (strcmp(line + 9, "0x82") == 0 ||
					strcmp(line + 9, "Songwriter") == 0)
				pack_type = 0x82;
			else if (strcmp(line + 9, "0x83") == 0 ||
					strcmp(line + 9, "Composer") == 0)
				pack_type = 0x83;
			else if (strcmp(line + 9, "0x84") == 0 ||
					strcmp(line + 9, "Arranger") == 0)
				pack_type = 0x84;
			else if (strcmp(line + 9, "0x85") == 0 ||
					strcmp(line + 9, "Message") == 0)
				pack_type = 0x85;
			else if (strcmp(line + 9, "0x8e") == 0 ||
					strcmp(line + 9, "ISRC") == 0) {
				pack_type = 0x8e;
				if (!(flag & 2)) {
					ret = burn_track_set_isrc_string(
						tracks[tnum], payload, 0);
					if (ret <= 0)
						goto ex;
				}
			} else {
				sprintf(msg,
				   "Unknown v07t Track purpose specifier '%s'",
				       line + 9);
				libdax_msgs_submit(libdax_messenger, -1,
					  0x00020191, LIBDAX_MSGS_SEV_FAILURE,
					  LIBDAX_MSGS_PRIO_HIGH,
					  burn_printify(msg), 0, 0);
				ret = 0; goto ex;
			}
			ret = v07t_cdtext_to_track(tracks[tnum], block,
					payload, &int0x00, pack_type, "", 0);
			if (ret <= 0)
				goto ex;
			track_attr_seen[pack_type - 0x80] = 1;

		} else if (strncmp(line, "ISRC ", 5) == 0) {
			/* Track variation of UPC EAN = 0x8e */
			tno = -1;
			sscanf(line + 5, "%d", &tno);
			if (tno <= 0 || tno - track_offset < 0 ||
				tno - track_offset >= num_tracks) {
				track_txt[0] = line[5];
				track_txt[1] = line[6];
				track_txt[2] = 0;
				goto bad_track_no;
			}
			tnum = tno - track_offset;
			if (!(flag & 2)) {
				ret = burn_track_set_isrc_string(
						tracks[tnum], payload, 0);
				if (ret <= 0)
					goto ex;
			}
			ret = v07t_cdtext_to_track(tracks[tnum], block,
					 payload, &int0x00, 0x8e, "", 0);
			if (ret <= 0)
				goto ex;
			track_attr_seen[0xe] = 1;

		} else {
			sprintf(msg,
				"Unknown v07t purpose specifier '%.4000s'",
				line);
			libdax_msgs_submit(libdax_messenger, -1, 0x00020191,
				LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
				burn_printify(msg), 0, 0);
			ret = 0; goto ex;
		}
	}
	ret = v07t_apply_to_session(session, block,
	                            char_codes, copyrights, languages,
	                            session_attr_seen, track_attr_seen,
	                            genre_code, genre_text, 0);
	if (ret <= 0)
		goto ex;

	ret = 1;
	if (additional_blocks > 0)
		ret += additional_blocks;;
ex:;
	if(fp != NULL)
		fclose(fp);
	BURN_FREE_MEM(genre_text);
	BURN_FREE_MEM(line);
	BURN_FREE_MEM(msg);
	return ret;
}


/* ts B11221 API */
int burn_cdtext_from_packfile(char *path, unsigned char **text_packs,
				 int *num_packs, int flag)
{
	int ret = 0, residue = 0;
	struct stat stbuf;
	FILE *fp = NULL;
	unsigned char head[4], tail[1];
	char *msg = NULL;

	BURN_ALLOC_MEM(msg, char, 4096);

	if (stat(path, &stbuf) == -1) {
cannot_open:;
		sprintf(msg, "Cannot open CD-TEXT pack file '%.4000s'", path);
		libdax_msgs_submit(libdax_messenger, -1, 0x00020198,
			LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
			burn_printify(msg), errno, 0);
		ret = 0; goto ex;
	}
	if (!S_ISREG(stbuf.st_mode))
		goto not_a_textfile;
	residue = (stbuf.st_size % 18);
	if(residue != 4 && residue != 0 && residue != 1) {
not_a_textfile:;
		sprintf(msg,
	  "File is not of usable type or content for CD-TEXT packs: '%.4000s'",
			path);
		libdax_msgs_submit(libdax_messenger, -1, 0x00020198,
			LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
			burn_printify(msg), 0, 0);
		ret = 0; goto ex;
	}
	if (stbuf.st_size < 18)
		goto not_a_textfile;

	fp = fopen(path, "rb");
	if (fp == NULL)
		goto cannot_open;
	if (residue == 4) { /* This is for files from cdrecord -vv -toc */
		ret = fread(head, 4, 1, fp);
		if (ret != 1) {
cannot_read:;
			sprintf(msg,
		      "Cannot read all bytes from CD-TEXT pack file '%.4000s'",
				path);
			libdax_msgs_submit(libdax_messenger, -1, 0x00020198,
				LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
				burn_printify(msg), errno, 0);
			ret = 0; goto ex;
		}
		if (head[0] * 256 + head[1] != stbuf.st_size - 2)
			goto not_a_textfile;
	}
	*num_packs = (stbuf.st_size - residue) / 18;
	if (*num_packs > 2048) {
		/* Each block can have 256 text packs.
		   There are 8 blocks at most. */
		sprintf(msg,
		   "CD-Text pack file too large (max. 36864 bytes): '%.4000s'",
			path);
		libdax_msgs_submit(libdax_messenger, -1, 0x0002018b,
				LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
				burn_printify(msg), errno, 0);
		ret = 0; goto ex;
	}

	BURN_ALLOC_MEM(*text_packs, unsigned char, *num_packs * 18);
	ret = fread(*text_packs, *num_packs * 18, 1, fp);
	if (ret != 1)
		goto cannot_read;
	if (residue == 1) { /* This is for Sony CDTEXT files */
		ret = fread(tail, 1, 1, fp);
		if (ret != 1)
			goto cannot_read;
		if (tail[0] != 0)
			goto not_a_textfile;
	}

	ret= 1;
ex:;
	if (ret <= 0) {
		BURN_FREE_MEM(*text_packs);
		*text_packs = NULL;
		*num_packs = 0;
	}
	if (fp != NULL)
		fclose(fp);
	BURN_FREE_MEM(msg);
	return ret;
}


/* --------------------------------- make_v07t -------------------------- */


static int search_pack(unsigned char *text_packs, int num_packs,
                       int start_no, int pack_type, int block,
                       unsigned char **found_pack, int *found_no, int flag)
{
	int i;

	for (i = start_no; i < num_packs; i++) {
		if (pack_type >= 0)
			if (text_packs[i * 18] != pack_type)
	continue;
		if (block >= 0)
			if (((text_packs[i * 18 + 3] >> 4) & 7) != block)
	continue;
		*found_pack = text_packs + i * 18;
		*found_no = i;
		return 1;
	}
	*found_pack = NULL;
	*found_no = num_packs;
	return 0;
}


static void write_v07t_line(char **respt, char *spec, char *value, int vlen,
                            int *result_len, int flag)
{
	int len;

	if (vlen == -1)
		vlen = strlen(value);
	len = strlen(spec);
	if (len < 19)
		len = 19;
	len += 3 + vlen + 1;
	if(flag & 1) {
		*result_len += len;
		return;
	}
	sprintf(*respt, "%-19s = ", spec);
	if (vlen > 0)
		memcpy(*respt + strlen(*respt), value, vlen);
	(*respt)[len - 1] = '\n';
	(*respt)[len] = 0;
	*respt+= len;
}


/*
    @return            -1 error
                        0 no pack of block,pack_type found
                        1 packs found, delimiter is single 0-byte
                        2 packs found, delimiter is double 0-byte
*/
static int collect_payload(unsigned char *text_packs, int num_packs,
                           int pack_type, int block,
                           unsigned char **payload, int *payload_count,
                           int flag)
{
	unsigned char *pack;
	int pack_no, ret, double_byte = 0;

	*payload_count = 0;
	for (pack_no = 0; ; pack_no++) {
		ret = search_pack(text_packs, num_packs, pack_no, pack_type,
		                  block, &pack, &pack_no, 0);
		if (ret <= 0)
	break;
		*payload_count += 12;
	}
	if (*payload_count == 0)
		return 0;
	*payload = burn_alloc_mem(*payload_count + 1, 1, 0);
	if (*payload == NULL)
		return -1;
	*payload_count = 0;
	for (pack_no = 0; ; pack_no++) {
		ret = search_pack(text_packs, num_packs, pack_no, pack_type,
		                  block, &pack, &pack_no, 0);
		if (ret <= 0)
	break;
		memcpy(*payload + *payload_count, pack + 4, 12);
		*payload_count += 12;
		if (pack[4] & 128)
			double_byte = 1;
	}
	(*payload)[*payload_count] = 0;
	return 1 + double_byte;
}


/*
    @param flag        bit0= use double 0 as delimiter
*/
static int is_payload_text_end(unsigned char *payload, int payload_count,
                               int i, int flag)
{
	if (i >= payload_count)
		return 1;
	if (payload[i])
		return 0;
	if (!(flag & 1))
		return 1;
	if (i + 1 >= payload_count)
		return 1;
	if (payload[i + 1] == 0)
		return 1;
	return 0;
}


/*
    @param flag        Bitfield for control purposes.
                       bit0= use double 0 as delimiter
                       bit1= replace TAB resp. TAB TAB by text of previous tno
*/
static int pick_payload_text(unsigned char *payload, int payload_count,
                             int tno,
                             unsigned char **text_start, int *text_len,
                             int flag)
{
	int i, skipped = 0, end_found = 0;

again:;
	if (tno <= 0) {
		*text_start = payload;
		*text_len = 0;
		for (i = 0; i < payload_count; i += 1 + (flag & 1)) {
			end_found = is_payload_text_end(payload, payload_count,
		                                        i, flag & 1);
			if (end_found) {
				*text_len = i;
		break;
			}
		}
		return 1;
	}
	*text_start = NULL;
	*text_len = 0;
	for (i = 0; i < payload_count; i += 1 + (flag & 1)) {
		end_found = is_payload_text_end(payload, payload_count,
		                                i, flag & 1);
		if (end_found) {
			skipped++;
			if (skipped == tno) {
				*text_start = payload + (i + 1 + (flag & 1));
			} else if (skipped == tno + 1) {
				*text_len = i - (*text_start - payload);
				goto found;
			}
		}
	}
	if (*text_start == NULL)
		return 0;
	*text_len = payload_count - (*text_start - payload);

found:;
	if (flag & 2) {
		/* If TAB resp. TAB TAB, then look back */
		if (flag & 1) {
			if (*text_len == 2) {
				if ((*text_start)[0] == '\t' &&
				    (*text_start)[1] == '\t') {
					skipped = 0;
					tno--;
					goto again;
				}
			}
		} else if (*text_len == 1) {
			if ((*text_start)[0] == '\t') {
				skipped = 0;
				tno--;
				goto again;
			}
		}
	}
	return 1;
}


static int write_v07t_textline(unsigned char *text_packs, int num_packs,
                               int pack_type, int block,
                               int tno, int first_tno, char *spec,
                               char **respt, int *result_len, int flag)
{
	unsigned char *payload = NULL, *text_start;
	int ret, payload_count = 0, text_len, tab_flag = 0;
	char msg[80];

	if ((pack_type >= 0x80 && pack_type <= 0x85) || pack_type == 0x8e)
		tab_flag = 2;
	ret = collect_payload(text_packs, num_packs, pack_type, block, 
                              &payload, &payload_count, 0);
	if(ret > 0) {
		ret = pick_payload_text(payload, payload_count, tno,
		                        &text_start, &text_len,
		                        (ret == 2) | tab_flag);
		if (ret > 0) {
			if (tno > 0 && strcmp(spec, "ISRC") == 0)
				sprintf(msg, "%s %-2.2d",
				              spec, tno + first_tno - 1);
			else if (tno > 0)
				sprintf(msg, "Track %-2.2d %s",
				              tno + first_tno - 1, spec);
			else
				strcpy(msg, spec);
			write_v07t_line(respt, msg,
			                (char *) text_start, text_len,
			                result_len, flag & 1);
			ret = 1;
		}
	}
	BURN_FREE_MEM(payload);
	return ret;
}


static int report_track(unsigned char *text_packs, int num_packs, 
                        int block, int tno, int first_tno,
                        char **respt, int *result_len, int flag)
{
	int ret, i;
	static char *track_specs[6] = {
		"Title", "Artist", "Songwriter", "Composer",
		"Arranger", "Message"
	};

	for (i = 0; i < 6; i++) {
		ret = write_v07t_textline(text_packs, num_packs, 0x80 + i,
		                          block, tno, first_tno,
		                          track_specs[i], respt, result_len,
	                                  flag & 1);
		if (ret < 0)
			return -1;
	}
	ret = write_v07t_textline(text_packs, num_packs, 0x8e, block,
	                          tno, first_tno,
	                          "ISRC", respt, result_len, flag & 1);
	if (ret < 0)
		return -1;
	return 1;
}


/*
    @param flag        Bitfield for control purposes.
                       bit0= Do not store text in result but only determine
                             the minimum size for the result array.
                             It is permissible to submit result == NULL.
                             Submit the already occupied size as result_size.
    @return            > 0 tells the number of valid text bytes in result resp.
                           with flag bit0 the prediction of that number.
                           This does not include the trailing 0-byte.
                       = 0 indicates that the block is not present
                       < 0 indicates failure.
*/
static int report_block(unsigned char *text_packs, int num_packs, 
                        int block, int first_tno, int last_tno, int char_code,
                        char *result, int result_size, int flag)
{
	char *respt = NULL;
	unsigned char *pack, *payload = NULL;
	int result_len = 0, pack_no, ret, i, lang, payload_count = 0, genre;
	char msg[80];
	static char *languages[] = {
		BURN_CDTEXT_LANGUAGES_0X00,
		BURN_CDTEXT_FILLER,
		BURN_CDTEXT_LANGUAGES_0X45
	};
	static char *volume_specs[7] = {
		"Album Title", "Artist Name", "Songwriter", "Composer",
		"Arranger", "Album Message", "Catalog Number",
	};
	static char *genres[BURN_CDTEXT_NUM_GENRES] = {
		BURN_CDTEXT_GENRE_LIST
	};

	/* Search for any pack of the block. But do not accept 0x8f as first.*/
	ret = search_pack(text_packs, num_packs, 0, -1, block, 
                          &pack, &pack_no, 0);
	if (ret <= 0)
		return 0;
	if (pack[0] == 0x8f)
		return 0;
	
	if (flag & 1) {
		result_len = result_size;
	} else {
		respt = result + result_size;
	}
	write_v07t_line(&respt, "Input Sheet Version", "0.7T", -1, &result_len,
	                flag & 1);
	sprintf(msg, "Libburn report of CD-TEXT Block %d", block);
	write_v07t_line(&respt, "Remarks            ", msg, -1, &result_len,
	                flag & 1);
	write_v07t_line(&respt, "Text Code          ",
	      char_code == 0 ? "8859" : char_code == 0x01 ? "ASCII" : "MS-JIS",
	                -1, &result_len, flag & 1);

	pack_no = 0;
	for (i = 0; i < 3; i++) {
		ret = search_pack(text_packs, num_packs, pack_no, 0x8f, -1, 
                                  &pack, &pack_no, 0);
		if (ret <= 0) {
			libdax_msgs_submit(libdax_messenger, -1, 0x0002019f,
			    LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
	    	  "No third CD-TEXT pack 0x8f found. No language code defined",
			    0, 0);
			goto failure;
		}
		pack_no++;
	}
	lang = pack[8 + block];
	if (lang > 127) {
		sprintf(msg, "CD-TEXT with unknown language code %2.2x",
		             (unsigned int) lang);
		libdax_msgs_submit(libdax_messenger, -1, 0x0002019f,
				LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
				msg, 0, 0);
		goto failure;
	}
	write_v07t_line(&respt, "Language Code", languages[lang], -1,
	                &result_len, flag & 1);

	for (i = 0; i < 7; i++) {
		ret = write_v07t_textline(text_packs, num_packs, 0x80 + i,
		                          block, 0, 0, volume_specs[i],
	                                  &respt, &result_len,
	                                  flag & 1);
		if (ret < 0)
			goto failure;
	}

	ret = collect_payload(text_packs, num_packs, 0x87, block, 
                              &payload, &payload_count, 0);
	if(ret > 0) {
		genre = (payload[0] << 8) | payload[1];
		if (genre < BURN_CDTEXT_NUM_GENRES) 
			strcpy(msg, genres[genre]);
		else
			sprintf(msg, "0x%-4.4x", (unsigned int) genre);
		write_v07t_line(&respt, "Genre Code", msg,
				-1, &result_len, flag & 1);
		write_v07t_line(&respt, "Genre Information",
		                (char *) payload + 2,
				-1, &result_len, flag & 1);
		BURN_FREE_MEM(payload); payload = NULL;
	}
	ret = collect_payload(text_packs, num_packs, 0x8d, block, 
                              &payload, &payload_count, 0);
	if(ret > 0) {
		write_v07t_line(&respt, "Closed Information", (char *) payload,
		                -1, &result_len, flag & 1);
		BURN_FREE_MEM(payload); payload = NULL;
	}
	ret = write_v07t_textline(text_packs, num_packs, 0x8e, block, 0, 0,
	                          "UPC / EAN", &respt, &result_len, flag & 1);
	if (ret < 0)
		goto failure;
	ret = search_pack(text_packs, num_packs, 0, 0x8f, -1, 
	                  &pack, &pack_no, 0);
	if (ret < 0)
		goto failure;
	if (pack[7] == 0x00)
		strcpy(msg, "OFF");
	else if (pack[7] == 0x03)
		strcpy(msg, "ON");
	else
		sprintf(msg, "0x%2.2x", (unsigned int) pack[7]);
	write_v07t_line(&respt, "Text Data Copy Protection", msg,
				-1, &result_len, flag & 1);
	sprintf(msg, "%d", first_tno);
	write_v07t_line(&respt, "First Track Number", msg,
		                -1, &result_len, flag & 1);
	sprintf(msg, "%d", last_tno);
	write_v07t_line(&respt, "Last Track Number", msg,
		                -1, &result_len, flag & 1);
	
	for (i = 0; i < last_tno - first_tno + 1; i++) {
		ret = report_track(text_packs, num_packs, block,
		                   i + 1, first_tno,
                                   &respt, &result_len, flag & 1);
		if (ret < 0)
			goto failure;
	}

	if (flag & 1)
		return result_len;
	return respt - result;

failure:;
	BURN_FREE_MEM(payload);
	return -1;
}


/*
    @param result      A byte buffer of sufficient size.
                       It will be filled by the text for the v07t sheet file
                       plus a trailing 0-byte. (Be aware that double-byte
                       characters might contain 0-bytes, too.)
    @param result_size The number of bytes in result.
                       To be determined by a run with flag bit0 set.
    @param flag        Bitfield for control purposes.
                       bit0= Do not store text in result but only determine
                             the minimum size for the result array.
                             It is permissible to submit result == NULL and
                             result_size == 0.
    @return            > 0 tells the number of valid text bytes in result resp.
                           with flag bit0 the prediction of that number.
                           This does not include the trailing 0-byte.
                       <= 0 indicates failure.
*/
static int burn_make_v07t(unsigned char *text_packs, int num_packs,
                          int first_tno, int track_count,
                          char *result, int result_size,
                          int *char_code, int flag)
{
	int pack_no = 0, ret, block, last_tno = 0;
	unsigned char *pack;
	char msg[80];

	/* >>> ??? Verify checksums ? */;

	/* Check character code, reject unknown ones */
	ret = search_pack(text_packs, num_packs, 0, 0x8f, -1, 
                          &pack, &pack_no, 0);
	if (ret <= 0) {
		libdax_msgs_submit(libdax_messenger, -1, 0x0002019f,
		    LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
		    "No CD-TEXT pack 0x8f found. No character code defined",
		    0, 0);
		return 0;
	}
	*char_code = pack[4];
	if (*char_code != 0x00 && *char_code != 0x01 && *char_code != 0x80) {
		sprintf(msg, "CD-TEXT with unknown character code %2.2x",
			    (unsigned int) *char_code);
		libdax_msgs_submit(libdax_messenger, -1, 0x0002019f,
				LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
				msg, 0, 0);
		return 0;
	}

	/* Obtain first_tno and last_tno from type 0x88 if present. */
	if (first_tno <= 0) {
		if (pack[5] > 0 &&  pack[5] + pack[6] < 100 &&
		    pack[5] <= pack[6]) {
			first_tno = pack[5];
			last_tno = pack[6];
		} else {
			sprintf(msg,
			        "CD-TEXT with illegal track range %d to %d",
			        (int) pack[5], (int) pack[6]);
			libdax_msgs_submit(libdax_messenger, -1, 0x0002019f,
				LIBDAX_MSGS_SEV_FAILURE, LIBDAX_MSGS_PRIO_HIGH,
				msg, 0, 0);
			return 0;
		}
	}
	if (last_tno <= 0) {
		if (track_count > 0) {
			last_tno = first_tno + track_count - 1;
		} else {
			last_tno = 99;
		}
	}

	/* Report content */
	result_size = 0;
	for (block = 0; block < 8; block++) {
		ret = report_block(text_packs, num_packs, block, 
		                   first_tno, last_tno, *char_code,
		                   result, result_size, flag & 1);
		if (ret < 0)
			return ret;
		if (ret == 0)
	continue;
		result_size = ret;
	}

#ifdef NIX
	if (flag & 1)
		return result_size;
	return (int) strlen((char *) result);
#else /* NIX */

	return result_size;

#endif /* ! NIX */
}


/*  Convert an array of CD-TEXT packs into the text format of
    Sony CD-TEXT Input Sheet Version 0.7T .

    @param text_packs  Array of bytes which form CD-TEXT packs of 18 bytes
                       each. For a description of the format of the array,
                       see file doc/cdtext.txt.
                       No header of 4 bytes must be prepended which would
                       tell the number of pack bytes + 2.
                       This parameter may be NULL if the currently attached
                       array of packs shall be removed.
    @param num_packs   The number of 18 byte packs in text_packs.
    @param start_tno   The start number of track counting, if known from
                       CD table-of-content or orther sources.
                       Submit 0 to enable the attempt to read it and the
                       track_count from pack type 0x8f.
    @param track_count The number of tracks, if known from CD table-of-content
                       or orther sources.
    @param result      Will return the buffer with Sheet text.
                       Dispose by free() when no longer needed.
                       It will be filled by the text for the v07t sheet file
                       plus a trailing 0-byte. (Be aware that double-byte
                       characters might contain 0-bytes, too.)
                       Each CD-TEXT language block starts by the line
                         "Input Sheet Version = 0.7T"
                       and a "Remarks" line that tells the block number.
    @param char_code   Returns the character code of the pack array:
                         0x00 = ISO-8859-1
                         0x01 = 7 bit ASCII
                         0x80 = MS-JIS (japanese Kanji, double byte characters)
                       The presence of a code value that is not in this list
                       will cause this function to fail.
    @param flag        Bitfield for control purposes. Unused yet. Submit 0.
    @return            > 0 tells the number of valid text bytes in result.
                           This does not include the trailing 0-byte.
                       <= 0 indicates failure.
*/
int burn_make_input_sheet_v07t(unsigned char *text_packs, int num_packs,
                               int start_tno, int track_count,
                               char **result, int *char_code, int flag)
{
	int ret, result_size = 0;

	ret = burn_make_v07t(text_packs, num_packs, start_tno, track_count,
	                     NULL, 0, char_code, 1);
	if (ret <= 0)
		return ret;
	result_size = ret + 1;
	*result = burn_alloc_mem(result_size, 1, 0);
	if (*result == NULL)
		return -1;
	ret = burn_make_v07t(text_packs, num_packs, start_tno, track_count,
                             *result, result_size, char_code, 0);
	if (ret <= 0) {
		free(*result);
		return ret;
	}
	return result_size - 1;
}


