import sys
import getopt
from typing import Iterable

class option:
    short_name : str | None = None
    long_name : str | None = None
    description : str = ""
    key : str | None = None

    def __init__(self, **kwargs):
        for k, v in kwargs.items():
            setattr(self, k, v)

class parse_result:
    flags : set[option]
    config : dict[option, str]
    arguments : list[str]

def parse_arguments(options : Iterable[option], args : Iterable[str] = sys.argv[1:]) -> parse_result:
    # Check for redefinitions
    short_definitions : dict[str, option] = {}
    long_definitions : dict[str, option] = {}
    key_definitions : dict[str, option] = {}
    for opt in options:
        if opt.short_name is not None:
            if opt.short_name in short_definitions:
                raise Exception("short name \"%s\" redefined" % opt.short_name, short_definitions[opt.short_name], opt)
            elif len(opt.short_name) != 1:
                raise Exception("detected invalid short name", opt)
            else:
                short_definitions[opt.short_name] = opt

        if opt.short_name is not None:
            if opt.long_name in long_definitions:
                raise Exception("long name \"%s\" redefined" % opt.short_name, long_definitions[opt.short_name], opt)
            elif len(opt.long_name) == 0:
                raise Exception("detected empty long name", opt)
            else:
                long_definitions[opt.long_name] = opt

        if opt.key is not None:
            if opt.key in key_definitions:
                raise Exception("key \"%s\" redefined" % opt.short_name, short_definitions[opt.short_name], opt)
            elif len(opt.key) == 0:
                raise Exception("detected empty key", opt)
            else:
                key_definitions[opt.key] = opt

        if opt.short_name is None and opt.long_name is None:
            raise Exception("option had no name", opt)


            

    optstr = "".join([opt.short_name + (opt.key is not None and ":" or '') for opt in options if opt.short_name is not None])
    longopts = [ opt.long_name for opt in options if opt.long_name is not None ]

    optlist, args = getopt.getopt(args, optstr, longopts)

    optset : set[option] = set()
    optdict : dict[str, option] = {}

    for item in optlist:
        text = item[0]
        if len(text) == 2:
            # Short item
            text = text[1]
            if text in short_definitions:
                optset.add(short_definitions[text])
            else:
                raise Exception("undefined short options '%s'" % item[0])
        elif len(text) > 2:
            # Long item
            text = text[2:]
            if text in long_definitions:
                opt = long_definitions[text]
                if opt.key is None:
                    optset.add(opt)
                elif opt.key in key_definitions:
                    optdict[opt.key] = item[1]
                else:
                    raise Exception("long option \"%s\" had invalid key \"%s\"" % (item[0], opt.key))
            else:
                raise Exception("undefined long option \"%s\"" % item[0])
        else:
            raise Exception("unexpected getopt() return")


    rt = parse_result()
    rt.flags = optset
    rt.config = optdict
    rt.arguments = args

    return rt