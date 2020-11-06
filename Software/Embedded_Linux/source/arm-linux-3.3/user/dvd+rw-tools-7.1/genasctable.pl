#!/bin/env perl

print <<___;
const char *ASC_lookup (int code)
{ static const struct { int code; const char *msg; } _table [] = {
___
while(<>)
{   chomp;
    if (m/([0-9A-F\-\/]+)\s+([0-9A-F]{2})\s+([0-9A-F]{2})\s+(.*)/i)
    {	printf "    { 0x%s%s, \"%s\" },\n",$2,$3,$4;	}
}
print <<___;
    { 0xFFFF, NULL }
  };
  int sz = sizeof(_table)/sizeof(_table[0])/2,i=sz;

    code &= 0xFFFF;

    while (sz)
    {	if (_table[i].code == code)	return _table[i].msg;

	if (_table[i].code > code)
	{   if (sz/=2)	i -= sz;
	    else	while (_table[--i].code > code);
	}
	else
	{   if (sz/=2)	i += sz;
	    else	while (_table[++i].code < code);
	}
    }

  return _table[i].code==code ? _table[i].msg : NULL;
}
___
