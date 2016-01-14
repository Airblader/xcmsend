// vim:ts=4:sw=4:expandtab
#pragma once

#define xmacro(atom) xcb_atom_t A_##atom;
#include "atoms.xmacro"
#undef xmacro
