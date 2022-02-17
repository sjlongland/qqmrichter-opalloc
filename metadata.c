/******************************************************************************
* (c)2022 Michael T. Richter
*
* This software is distributed under the terms of WTFPLv2.  The full terms and
* text of the license can be found at http://www.wtfpl.net/txt/copying
******************************************************************************/

// Project information file for keeping all metadata in the same place.
__attribute__((visibility("default"))) const char     PROJECT_NAME[]       = "OPALLOC";
__attribute__((visibility("default"))) const char     PROJECT_OID_STRING[] =
    "{ iso(1) org(3) dod(5) internet(1) private(4) enterprise(1) half-baked(45340) projects(1) software(1) opalloc(2) }";
__attribute__((visibility("default"))) const char     PROJECT_OID_DOTTED[] = "1.3.5.1.4.1.45340.1.1.2";
__attribute__((visibility("default"))) const char     PROJECT_OID_BER[]    =
{
    0x06, 0x0b, 0x2b, 0x05, 0x01, 0x04, 0x01, 0x82, 0xe2, 0x1c, 0x01, 0x01, 0x02
};
__attribute__((visibility("default"))) const unsigned PROJECT_OID_BER_SIZE = sizeof(PROJECT_OID_BER);
