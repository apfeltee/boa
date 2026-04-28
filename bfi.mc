
var BFI_MAXMEMORY = 30000
var TOK_BRACOPEN = "[".byteAt(0)
var TOK_BRACCLOSE = "]".byteAt(0)
var TOK_DOT = ".".byteAt(0)
var TOK_COMMA = ",".byteAt(0)
var TOK_LESS = "<".byteAt(0)
var TOK_GREATER = ">".byteAt(0)
var TOK_PLUS = "+".byteAt(0)
var TOK_MINUS = "-".byteAt(0)

function bf(bfsrc, input)
{
    var i
    var memory = []
    /* foreach i in 0..300 */
    //for(i=0; i<300; i+=1)
    for(i=0; i<BFI_MAXMEMORY; i+=1)
    {
        memory.push(0)
    }
    var memptr  = 0
    var inptr = 0
    var instr    = 0
    var stack = []
    while(instr < bfsrc.length)
    {
        if(instr < 0)
        {
            Error("Invalid instruction pointer (instr=" + instr + ")")
        }
        var c = bfsrc.byteAt(instr)
        if(c == TOK_PLUS)
        {
            memory[memptr] = memory[memptr] + 1
            if(memptr >= memory.length)
            {
                Error("Heap overrun")
            }
        }
        else if(c == TOK_MINUS)
        {
            memory[memptr] = memory[memptr] - 1
            if(memptr < 0)
            {
                Error("Heap underrun")
            }
        }
        else if(c == TOK_DOT)
        {
            STDOUT.putc(memory[memptr])
        }
        else if(c == TOK_COMMA)
        {
           memory[memptr] = STDIN.readchar()
           inptr = inptr + 1
        }
        else if(c == TOK_GREATER)
        {
            memptr = memptr + 1
            if(memptr > memory.length)
            {
                Error("data pointer out of bounds")
            }
        }
        else if(c == TOK_LESS)
        {
            memptr = memptr - 1
            if(memptr < 0)
            {
                //Error("data pointer cannot go below 0")
                memptr = 0
            }
        }
        else if(c == TOK_BRACOPEN)
        {
            if (memory[memptr] != 0)
                stack.append(instr)
            else
            {
                var bcount = 0
                var cont = true
                while(cont)
                {
                    instr = instr + 1
                    if(instr > bfsrc.length)
                    {
                        Error("Missing matching ']'")
                    }
                    if(bfsrc.byteAt(instr) == TOK_BRACCLOSE)
                    {
                        if(bcount != 0)
                        {
                            bcount = bcount - 1;
                        }
                        else
                        {
                            cont = false;
                        }
                    }
                    else if(bfsrc.byteAt(instr) == TOK_BRACOPEN)
                    {
                        bcount = bcount + 1;
                    }
               }
           }
        }
        else if(c == TOK_BRACCLOSE)
        {
            instr = stack.pop() - 1
        }
        instr = instr + 1
    }
}

function main(argv)
{
    var src = "++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++."
    if(argv.length > 1)
    {
        src = ""
        var fh = File(argv[1], "r")
        while(true)
        {
            var line = fh.readline()
            if(line == null)
            {
                break;
            }
            src += line
        }
    }
    println("bfsource: ", src)
    bf(src, "")
}

main(ARGV)
