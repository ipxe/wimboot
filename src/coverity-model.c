/*
 * Coverity modelling file
 *
 */

/* Inhibit use of built-in models for functions where Coverity's
 * assumptions about the modelled function are incorrect for wimboot.
 */
int getchar ( void ) {
}
