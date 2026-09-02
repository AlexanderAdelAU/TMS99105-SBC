/*
** NAMEIO.C -- resident filename prompt for Step 6.2.
**
** Filename entry is deliberately small and terminal-safe: printable non-space
** characters append, Backspace/Delete erase the last character, Ctrl-U clears,
** Enter accepts and Esc cancels.  The SBC file system uses short filenames, so
** input is capped at 14 characters (drive prefix plus 8.3 name).  Alphabetic
** input is normalized to uppercase because the SBC directory search compares FCB
** bytes literally.
*/

extern puts();
extern trmclear();
extern trmrev();
extern trmnorm();
extern trmpos();
extern trmclrln();
extern trmkey();
extern trmbell();
extern trmshow();
extern char *fsname;

nmprompt(label, buf)
char *label;
char *buf;
{
    int c;
    int len;

    len = 0;
    buf[0] = 0;

    trmshow();
    trmclear();
    trmrev();
    puts(" FILESTAT TEXT EDITOR - FILENAME ");
    trmclrln();
    trmnorm();
    puts("\n\n");

    if(fsname[0] != 0) {
        puts(" Current file: ");
        puts(fsname);
        puts("\n\n");
    }
    else
        puts("\n\n");

    puts(" Max 14 characters.  Enter accepts, Esc cancels, Ctrl-U clears.\n\n");

    while(1) {
        trmpos(8, 1);
        trmclrln();
        puts(label);
        puts(buf);

        c = trmkey();

        if(c == 27)
            return 0;

        if(c == 13 || c == 10) {
            if(len > 0)
                return 1;
            trmbell();
        }
        else if(c == 8 || c == 127) {
            if(len > 0) {
                len = len - 1;
                buf[len] = 0;
            }
            else
                trmbell();
        }
        else if(c == 21) {
            len = 0;
            buf[0] = 0;
        }
        else if(c >= 33 && c < 127) {
            if(len < 14) {
                if(c >= 'a' && c <= 'z')
                    c = c - 'a' + 'A';
                buf[len] = c;
                len = len + 1;
                buf[len] = 0;
            }
            else
                trmbell();
        }
        else
            trmbell();
    }
}
