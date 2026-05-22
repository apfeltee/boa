
class Cons {
    constructor(hd, tl) {
        if (hd == undefined && tl == undefined) {
            this.isnil = true;
        } else if (hd != undefined && tl != undefined) {
            this.isnil = false;
            this.head = hd;
            this.tail = tl;
        } else {
            throw Error('Error: Cons(' + hd + ', ' + tl + ')');
        }
    }
    toString() {
        if (this.isnil) return '()';
        return '(' + this.head.toString() + ' : ' + this.tail.toString() + ')';
    }

    is_nil() {
        return this.isnil;
    }
    car() {
        return this.head;
    }
    cdr() {
        return this.tail;
    }

    cadr() {
        return this.tail.head;
    }
    cddr() {
        return this.tail.tail;
    }
}

function arr2cons(arr) {
    var lst = new Cons();
    while (arr.length > 0)
        lst = new Cons(arr.pop(), lst);
    return lst;
}

function cons(hd, tl) {
    return new Cons(hd, tl);
}


function nil() {
    return new Cons();
}


COMMANDSET = {
    'LDC': function(secd) {
        var val = secd.pop_ctrl();
        secd.debug('LDC ' + val);
        secd.push_stack(val);
    },
    'ADD': function(secd) {
        var a = secd.pop_stack();
        var b = secd.pop_stack();
        secd.debug('ADD ' + a + ' ' + b);
        secd.push_stack(a + b);
    },
    'SUB': function(secd) {
        var a = secd.pop_stack();
        var b = secd.pop_stack();
        secd.debug('SUB ' + a + ' ' + b);
        secd.push_stack(a - b);
    },
    'MUL': function(secd) {
        var a = secd.pop_stack();
        var b = secd.pop_stack();
        secd.debug('MUL ' + a + ' ' + b);
        secd.push_stack(a * b);
    },
    'EQ': function(secd) {
        var a = secd.pop_stack();
        var b = secd.pop_stack();
        secd.debug('EQ ' + a + ' ' + b);
        secd.push_stack(a == b ? 1 : 0);
    },
    'CONS': function(secd) {
        var hd = secd.pop_stack();
        var tl = secd.pop_stack();
        secd.debug('CONS ' + hd + '        ' + tl);
        secd.push_stack(cons(hd, tl));
    },
    'SEL': function(secd) {
        var thenb = secd.pop_ctrl();
        var elseb = secd.pop_ctrl();
        var cond = secd.pop_stack();

        secd.push_dump(secd.ctrl);
        if (cond) {
            secd.debug('SEL then');
            secd.ctrl = thenb;
        } else {
            secd.debug('SECD else');
            secd.ctrl = elseb;
        }
    },
    'JOIN': function(secd) {
        secd.debug('JOIN');
        secd.ctrl = secd.pop_dump();
    },
    'LDF': function(secd) {
        var fundef = secd.pop_ctrl();
        var clos = arr2cons([fundef, secd.env]);
        secd.debug('LDF (' + fundef.car() + ') ...');
        secd.push_stack(clos);
    },
    'AP': function(secd) {
        var clos = secd.pop_stack();
        var argv = secd.pop_stack();

        var fundef = clos.car();
        var cenv = clos.cdr();

        var args = fundef.car();
        var functrl = fundef.cadr();

        secd.debug('AP');
        // push current continuation
        secd.push_dump(secd.ctrl);
        secd.push_dump(secd.env);
        secd.push_dump(secd.stack);

        secd.stack = nil();
        secd.env = cons(secd.new_frame(args, argv), cenv);
        secd.ctrl = functrl;

        secd.print_env();
    },
    'LD': function(secd) {
        var sym = secd.pop_ctrl();
        secd.debug('LD ' + sym);
        var val = secd.lookup(sym);
        if (val == undefined)
            println('LD: ' + sym + ' not found');
        secd.push_stack(val);
    },
    'RTN': function(secd) {
        var stack = secd.pop_dump();
        var env = secd.pop_dump();
        var ctrl = secd.pop_dump();

        var res = secd.pop_stack();
        secd.debug('RTN ' + res);

        secd.stack = cons(res, stack);
        secd.env = env;
        secd.ctrl = ctrl;
    },
    'DUM': function(secd) {
        secd.debug('DUM');
        secd.env = cons(nil(), secd.env);
    },
    'RAP': function(secd) {
        var clos = secd.pop_stack();
        var argv = secd.pop_stack();

        var fundef = clos.car();
        var cenv = clos.cdr();

        var args = fundef.car();
        var functrl = fundef.cadr();

        cenv.head = secd.new_frame(args, argv);
        secd.debug('RAP');

        // push current continuation
        secd.push_dump(secd.ctrl);
        secd.push_dump(secd.env.cdr());
        secd.push_dump(secd.stack);

        secd.stack = nil();
        secd.env = cenv;
        secd.ctrl = functrl;

        secd.print_env();
    },
};


class Secd
{
    constructor(ctrl) {
        this.stack = nil();
        this.env = nil(); // TODO
        this.ctrl = ctrl;
        this.dump = nil();
        return this;
    }

    debug(msg) {
        println(msg);
        /*
        println('S = ' + this.stack);
        println('E = ' + this.env);
        println('C = ' + this.ctrl);
        println('D = ' + this.dump);
        // */
    }

    pop_ctrl() {
        var hd = this.ctrl.car();
        this.ctrl = this.ctrl.cdr();
        return hd;
    }

    pop_stack() {
        var hd = this.stack.car();
        this.stack = this.stack.cdr();
        return hd;
    }
    push_stack(val) {
        this.stack = cons(val, this.stack);
    }

    pop_dump() {
        var hd = this.dump.car();
        this.dump = this.dump.cdr();
        return hd;
    }
    push_dump(val) {
        return (this.dump = cons(val, this.dump));
    }

    new_frame(args, argv) {
        return cons(args, argv);
    }
    lookup(sym) {
        var e = this.env;
        while (!e.is_nil()) {
            var frame = e.car();
            var args = frame.car();
            var argv = frame.cdr();
            while (!args.is_nil()) {
                if (args.car() == sym)
                    return argv.car();
                args = args.cdr();
                argv = argv.cdr();
            }
            e = e.cdr();
        }
        return undefined;
    }
    print_env() {
        var e = this.env;
        while (!e.is_nil()) {
            var frame = e.car();
            println('Frame:');
            var args = frame.car();
            var argv = frame.cdr();
            while (!args.is_nil()) {
                println('   ' + args.car() + '\t -> ' + argv.car().toString());
                args = args.cdr();
                argv = argv.cdr();
            }
            e = e.cdr();
        }
    }
    run() {
        while (true) {
            var op = this.pop_ctrl();
            if (op == 'STOP') {
                println('_______________________');
                println('Result is ' + this.pop_stack());
                break;
            }
            if (!op) {
                println('unexpected end of commands');
                break;
            }
            var opcode = COMMANDSET[op];
            if (!opcode) {
                println('Error: no function for ' + op);
                break;
            }

            opcode(this);
        }
        println('\nSECD state:');
        println('stack = ' + this.stack);
        println('env = ' + this.env);
    }

}


var rectest = arr2cons(
    ["DUM", "LDC", nil(),
        "LDF", arr2cons([arr2cons(['x']), arr2cons([
            'LDC', 0, 'LD', 'x', 'EQ',
            'SEL', arr2cons(['LDC', 1, 'JOIN']),
            arr2cons(['LD', 'x',
                'LDC', nil(),
                'LDC', '1', 'LD', 'x', 'SUB',
                'CONS',
                'LD', 'fact', 'AP',
                'MUL', 'JOIN'
            ]),
            'RTN'
        ])]),
        "CONS",
        "LDF", arr2cons([arr2cons(['fact']), arr2cons([
            'LDC', nil(), 'LDC', 8, 'CONS',
            'LD', 'fact', 'AP', 'RTN',
        ])]),
        "RAP", "STOP"
    ]);

println("rectest = <<<", rectest, ">>>")
var secd = new Secd(rectest);
secd.run();

