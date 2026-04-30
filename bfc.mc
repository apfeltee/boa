
/*
* optimizing brainfuck compiler. currently runs faster than the interpreter, but
* still MUCH MUCH MUCH slower than the C version.
* biggest issue is dictionary lookup; it's very inefficient atm.
* curiously, running this script with nodejs (./noderun.sh bfc.nn hanoi.bf) shows
* that even node, a tweaked out JITted monster of an interpreter, doesn't run
* any better either. so it seems that ...
*   either current dictionary optimizations (prefer string lookup without string copying) is
*   not actually quite good enough, or
*   that the silly, ad-hoc table impl from Lox really is kinda terrible.
*
* frankly, i'm tending toward the latter. the Table impl is just really primitive and
* not very good.
*/

var BFC_MAXMEMORY = 30000;

var OPCODE_MOVE = 1;
var OPCODE_ADD = 2;
var OPCODE_PRINTBYTE = 3;
var OPCODE_READBYTE = 4;
var OPCODE_JUMPIFFALSE = 5;
var OPCODE_JUMPIFTRUE = 6;

var FIELDTYPE_OPER = 0;
var FIELDTYPE_ARGUMENT = 1;

function makefilledarray(cnt, val)
{
    var i;
    var rt = [];
    for(i=0; i<cnt; i+=1)
    {
        rt.push(val);
    }
    return rt;
}

function getchar()
{
    return null;
}

class BFRunner
{
    constructor()
    {
        this.compiledcode = [];
        this.compiledcount = 0;
        this.sourcepos = 0;
    }

    /*
    * NB. the C version uses a fixed-size array for the bytecode;
    * in a language with dynamic arrays, that isn't needed, obviously.
    * so, compile() returns the array of bytecode.
    */
    compile(program, len)
    {
        var depth;
        var target;
        var c;
        var i;
        var mval;
        var tmpop;
        var nextop;
        var pvop;
        while(this.sourcepos < len)
        {
            c = program[this.sourcepos];
            this.sourcepos+=1;
            if(c == ">" || c == "<")
            {
                if((this.compiledcount > 0) && (this.compiledcode[this.compiledcount - 1][FIELDTYPE_OPER] == OPCODE_MOVE))
                {
                    // NOTE: may be truncated on assignment to arg
                    // coalesce
                    mval = -1;
                    if(c == ">")
                    {
                        mval = 1;
                    }
                    var idx1 = this.compiledcount - 1
                    var idx2 = FIELDTYPE_ARGUMENT;
                    this.compiledcode[idx1][idx2] += mval;
                }
                else
                {
                    pvop = OPCODE_MOVE;
                    mval = -1;
                    if(c == ">")
                    {
                        mval = 1;
                    }
                    this.compiledcode.push([OPCODE_MOVE, mval])
                    this.compiledcount+=1;
                }
            }
            else if(c == "+" || c == "-")
            {
                if((this.compiledcount > 0) && (this.compiledcode[this.compiledcount - 1][FIELDTYPE_OPER] == OPCODE_ADD))
                {
                    // NOTE: may be truncated on assignment to arg
                    // coalesce
                    mval = -1;
                    if(c == "+")
                    {
                        mval = 1;
                    }
                    this.compiledcode[this.compiledcount - 1][FIELDTYPE_ARGUMENT] += mval;
                }
                else
                {
                    mval = -1;
                    if(c == "+")
                    {
                        mval = 1;
                    }
                    this.compiledcode.push([OPCODE_ADD, mval])
                    this.compiledcount+=1;
                }
            }
            else if(c == ".")
            {
                this.compiledcode.push([OPCODE_PRINTBYTE, 0])
                this.compiledcount+=1;
            }
            else if(c == ",")
            {
                this.compiledcode.push([OPCODE_READBYTE, 0])
                this.compiledcount+=1;
            }
            else if(c == "[")
            {
                this.compiledcode.push([OPCODE_JUMPIFFALSE, 0])
                this.compiledcount+=1;
            }
            else if(c == "]")
            {
                this.compiledcode.push([OPCODE_JUMPIFTRUE, 0])
                this.compiledcount+=1;
            }
        }
        println("compiled ", this.compiledcount, " instructions. now resolving jumps...")
        // Resolve all jumps
        for(i = 0; i < this.compiledcount; i+=1)
        {
            tmpop = this.compiledcode[i][FIELDTYPE_OPER];
            if(tmpop == OPCODE_JUMPIFFALSE)
            {
                depth = 1;
                target = i + 1;
                while((depth > 0) && (target < this.compiledcount))
                {
                    nextop = this.compiledcode[target][FIELDTYPE_OPER];
                    target+=1;
                    if(nextop == OPCODE_JUMPIFFALSE)
                    {
                        depth+=1;
                    }
                    else if(nextop == OPCODE_JUMPIFTRUE)
                    {
                        depth-=1;
                    }
                }
                if(depth > 0)
                {
                    // invalid program
                    return false;
                }
                this.compiledcode[i][FIELDTYPE_ARGUMENT] = target;
            }
            else if(tmpop == OPCODE_JUMPIFTRUE)
            {
                depth = 1;
                target = i;
                while((depth > 0) && target >= 0)
                {
                    target-=1;
                    nextop = this.compiledcode[target][FIELDTYPE_OPER];
                    if(nextop == OPCODE_JUMPIFTRUE)
                    {
                        depth+=1;
                    }
                    else if(nextop == OPCODE_JUMPIFFALSE)
                    {
                        depth-=1;
                    }
                }
                if(depth > 0)
                {
                    // invalid program
                    return false;
                }
                this.compiledcode[i][FIELDTYPE_ARGUMENT] = target + 1;
            }
        }
        println("finished compiling!")
        return true;
    }

    // Run a compiled bytecode program.
    run()
    {
        var datapos;
        var codepos;
        var maxcodecount = this.compiledcode.length;
        var mem = makefilledarray(BFC_MAXMEMORY, 0)
        //var mem = []
        datapos = 0;
        codepos = 0;
        println("running ", maxcodecount, " instructions...")
        while(codepos < maxcodecount)
        {
            var arg = this.compiledcode[codepos][FIELDTYPE_ARGUMENT];
            var opc = this.compiledcode[codepos][FIELDTYPE_OPER];
            codepos+=1;
            if(opc == OPCODE_MOVE)
            {
                datapos = mod(((datapos + arg) + BFC_MAXMEMORY), BFC_MAXMEMORY);
            }
            else if(opc == OPCODE_ADD)
            {
                mem[datapos] += arg;
            }
            else if(opc == OPCODE_JUMPIFFALSE)
            {
                if(mem[datapos] > 0)
                {
                    codepos = codepos;
                }
                else
                {
                    codepos = arg;
                }
            }
            else if(opc == OPCODE_JUMPIFTRUE)
            {
                if(mem[datapos] > 0)
                {
                    codepos = arg;
                }
                else
                {
                    codepos = codepos;
                }
            }
            else if(opc == OPCODE_PRINTBYTE)
            {
                STDOUT.putc(mem[datapos]);
            }
            else if(opc == OPCODE_READBYTE)
            {
                //STDOUT.flush()
                var c = STDIN.readchar();
                if(c == null)
                {
                    mem[datapos] = 0;
                }
                else
                {
                    mem[datapos] = c;
                }
            }
        }
    }

    optoname(op)
    {
        if(op == OPCODE_MOVE)
        {
            return "move";
        }
        else if(op == OPCODE_ADD)
        {
            return "add";
        }
        else if(op == OPCODE_PRINTBYTE)
        {
            return "printbyte";
        }
        else if(op == OPCODE_READBYTE)
        {
            return "readbyte";
        }
        else if(op == OPCODE_JUMPIFFALSE)
        {
            return "jumpiffalse";
        }
        else if(op == OPCODE_JUMPIFTRUE)
        {
            return "jumpifnotzero";
        }
        return null;
    }

    printbc()
    {
        var i;
        var brline = 0;
        var len = this.compiledcode.length;
        print("compiled: ", len, " items: [\n");
        for(i=0; i<len; i+=1)
        {
            var nopt = this.compiledcode[i][FIELDTYPE_OPER];
            var arg = this.compiledcode[i][FIELDTYPE_ARGUMENT];
            var optn = this.optoname(nopt);
            print("", optn, "(", arg, ")");
            if((i+1) < len)
            {
                print(", ");
            }
            if(brline == 10)
            {
                println();
                brline = 0;
            }
            brline+=1;
        }
        print("\n]\n");
    }
}

function main(argv)
{
    println("argv = ", argv)
    var src = "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++."
    var bfr = BFRunner()
    println("src=", src)
    if(!bfr.compile(src, src.length))
    {
        println("failed to compile program");
    }
    bfr.printbc();
    bfr.run();
}

main(ARGV);

