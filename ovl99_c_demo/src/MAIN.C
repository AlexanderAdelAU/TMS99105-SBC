/*
** MAIN.C -- the application.
**
** THE POINT OF THE DEMO: nothing in this file knows that hello() and
** farewell() are not in memory.  There is no overlay ID here, no
** R_xxx shim, no extern, no mapping call.  They are called exactly
** as any other function is, and a generated thunk of the same name
** maps the overlay they live in.
**
** The one line that is overlay-aware is ovlinit(), which brings the
** runtime up before any overlay is entered.
*/

main()
{
    ovlinit();

    pstr("OVL99 overlay demo\n");

    hello();                    /* overlay GREET, physical page 4 */
    farewell();                 /* overlay BYE,   physical page 5 */

    pstr("demo complete\n");
}
