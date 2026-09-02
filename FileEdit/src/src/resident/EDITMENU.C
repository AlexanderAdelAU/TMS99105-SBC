/*
** EDITMENU.C -- resident FileEdit shell (Step 7).
**
** The editor now owns a persistent current document across menu commands.
** Open, New, Save and Save As are normal application operations; the mapper,
** buffer address and capacity are deliberately absent from the user interface.
*/

extern puts();
extern fsreset();
extern fsopen();
extern fsclose();
extern fsrewind();
extern fsprobe();
extern fssave();
extern fssavas();
extern fsequal();
extern runword();
extern runchar();
extern runrpt();
extern viewctl();
extern editctl();
extern ebload();
extern ebloadn();
extern ebnewn();
extern ebnone();
extern nmprompt();
extern trmclear();
extern trmrev();
extern trmnorm();
extern trmesc();
extern trmkey();

extern char *fsname;
extern int ebvalid;
extern int ebdirty;
extern int fsexist;

/* Pause after a message/report so it remains visible. */
pausekey()
{
    puts("\nPress any key to return to the menu...");
    trmkey();
}

nodoc()
{
    trmclear();
    puts("No document is open.  Use Open or New first.\n");
    pausekey();
}

/*
** Protect a document before Open/New/Quit.  Returning to the menu from the
** editor itself never discards RAM changes; only a document transition asks.
*/
docguard()
{
    int c;

    if(ebvalid == 0)
        return 1;
    if(ebdirty == 0 && fsexist)
        return 1;

    trmclear();
    if(fsexist)
        puts("Unsaved changes in ");
    else
        puts("New document has not been saved: ");
    puts(fsname);
    puts("\n\n");
    puts("  S  Save and continue\n");
    puts("  D  Don't save\n");
    puts("  C  Cancel\n\n");
    puts(" Choice: ");

    while(1) {
        c = trmkey();
        if(c == 'S' || c == 's') {
            trmclear();
            puts("Saving ");
            puts(fsname);
            puts(" ...\n");
            if(fssave())
                return 1;
            puts("\nSave did not complete.  Document remains in memory.\n");
            pausekey();
            return 0;
        }
        if(c == 'D' || c == 'd')
            return 1;
        if(c == 'C' || c == 'c' || c == 27)
            return 0;
    }
}

/* Confirm Save As replacement when the requested target already exists. */
overask(name)
char *name;
{
    int c;

    trmclear();
    puts(name);
    puts(" already exists.\n\n");
    puts("A backup will be made before replacement.\n\n");
    puts("  R  Replace file\n");
    puts("  C  Cancel\n\n");
    puts(" Choice: ");

    while(1) {
        c = trmkey();
        if(c == 'R' || c == 'r')
            return 1;
        if(c == 'C' || c == 'c' || c == 27)
            return 0;
    }
}

/* Edit the current persistent document. */
editone()
{
    if(ebvalid == 0) {
        nodoc();
        return;
    }
    editctl();
}

/* View the current RAM document, including unsaved edits. */
viewone()
{
    if(ebvalid == 0) {
        nodoc();
        return;
    }
    viewctl();
}

/* Explicit failed-open recovery: never consume a future menu command as a pause. */
opretry(name)
char *name;
{
    int c;

    trmclear();
    puts("Cannot open ");
    puts(name);
    puts("\n\n");
    puts("  R  Retry filename\n");
    puts("  C  Cancel and return to menu\n\n");
    puts(" Choice: ");

    while(1) {
        c = trmkey();
        if(c == 'R' || c == 'r' || c == 13)
            return 1;
        if(c == 'C' || c == 'c' || c == 27)
            return 0;
    }
}

/* Open another file without destroying the current document on failure. */
openone()
{
    char name[16];
    int ready;

    ready = 0;
    while(ready == 0) {
        if(nmprompt(" Open file: ", name) == 0)
            return;

        if(fsprobe(name))
            ready = 1;
        else if(opretry(name) == 0)
            return;
    }

    if(docguard() == 0)
        return;

    trmclear();
    puts("Opening ");
    puts(name);
    puts(" ...\n");
    if(ebloadn(name) == 0) {
        puts("\nThe previous document is still intact.\n");
        pausekey();
    }
}

/* Create a new named document in RAM; disk creation waits for Save. */
newone()
{
    char name[16];

    if(nmprompt(" New file: ", name) == 0)
        return;

    if(fsprobe(name)) {
        trmclear();
        puts(name);
        puts(" already exists.  Use Open instead.\n");
        pausekey();
        return;
    }

    if(docguard() == 0)
        return;

    ebnewn(name);
}

/* Save the current document under its current name. */
saveone()
{
    if(ebvalid == 0) {
        nodoc();
        return;
    }

    trmclear();
    puts("Saving ");
    puts(fsname);
    puts(" ...\n");

    if(fssave()) {
        puts("\nSaved and verified: ");
        puts(fsname);
        puts("\n");
    }
    else
        puts("\nSave failed.  The document remains in memory.\n");

    pausekey();
}

/* Save the current RAM document under another filename. */
asone()
{
    char name[16];

    if(ebvalid == 0) {
        nodoc();
        return;
    }

    if(nmprompt(" Save as: ", name) == 0)
        return;

    if(fsequal(fsname, name)) {
        saveone();
        return;
    }

    if(fsprobe(name)) {
        if(overask(name) == 0)
            return;
    }

    trmclear();
    puts("Saving as ");
    puts(name);
    puts(" ...\n");

    if(fssavas(name)) {
        puts("\nSaved and verified: ");
        puts(fsname);
        puts("\n");
    }
    else
        puts("\nSave As failed.  Current document name and RAM are unchanged.\n");

    pausekey();
}

/* Run the original FILESTAT analysis against the saved disk copy. */
statone()
{
    int c;

    if(ebvalid == 0) {
        nodoc();
        return;
    }

    if(fsexist == 0) {
        trmclear();
        puts("Save this new document before running Statistics.\n");
        pausekey();
        return;
    }

    if(ebdirty) {
        trmclear();
        puts("This document has unsaved changes.\n\n");
        puts("  S  Save first, then calculate statistics\n");
        puts("  D  Use the last saved disk version\n");
        puts("  C  Cancel\n\n");
        puts(" Choice: ");

        while(1) {
            c = trmkey();
            if(c == 'S' || c == 's') {
                trmclear();
                puts("Saving ");
                puts(fsname);
                puts(" ...\n");
                if(fssave() == 0) {
                    puts("\nSave failed; statistics cancelled.\n");
                    pausekey();
                    return;
                }
                c = 0;
                break;
            }
            if(c == 'D' || c == 'd') {
                c = 0;
                break;
            }
            if(c == 'C' || c == 'c' || c == 27)
                return;
        }
    }

    fsreset();

    if(fsopen() == 0) {
        pausekey();
        return;
    }

    runword();

    if(fsrewind() == 0) {
        pausekey();
        return;
    }

    runchar();
    fsclose();

    trmclear();
    runrpt();
    pausekey();
}

/* Draw one complete Tera Term/ANSI menu frame. */
drawmenu()
{
    trmclear();

    trmrev();
    puts(" FILEEDIT - STEP 7");
    trmesc("[K");
    trmnorm();
    puts("\n\n");

    puts(" Current file: ");
    if(ebvalid) {
        puts(fsname);
        if(fsexist == 0)
            puts("  [NEW]");
        if(ebdirty)
            puts("  [MOD]");
    }
    else
        puts("(none)");
    puts("\n\n");

    puts("   E  Edit\n");
    puts("   O  Open\n");
    puts("   N  New\n");
    puts("   S  Save\n");
    puts("   A  Save As\n");
    puts("   V  View\n");
    puts("   T  Statistics\n");
    puts("   Q  Quit\n\n");

    trmrev();
    puts(" E Edit  O Open  N New  S Save  A SaveAs  V View  T Stats  Q Quit");
    trmesc("[K");
    trmnorm();
    puts("\n Command: ");
}

/* Persistent editor-menu lifetime. */
editmenu()
{
    int c;
    int done;

    ebnone();
    if(fsname[0] != 0) {
        trmclear();
        puts("Opening ");
        puts(fsname);
        puts(" ...\n");
        if(ebload() == 0)
            pausekey();
    }

    done = 0;
    while(done == 0) {
        drawmenu();
        c = trmkey();

        if(c == 'E' || c == 'e' || c == '1')
            editone();
        else if(c == 'O' || c == 'o')
            openone();
        else if(c == 'N' || c == 'n')
            newone();
        else if(c == 'S' || c == 's')
            saveone();
        else if(c == 'A' || c == 'a')
            asone();
        else if(c == 'V' || c == 'v')
            viewone();
        else if(c == 'T' || c == 't')
            statone();
        else if(c == 'Q' || c == 'q') {
            if(docguard())
                done = 1;
        }
    }

    trmclear();
}
