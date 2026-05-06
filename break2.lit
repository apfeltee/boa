{
    var a = 0;
    while(a < 5)
    {
        if (a == 3)
        {
            break;
        }
        println("simple break: a = ", a);
        a++;
    }
    println("after simple break\n");
}
"---end---";
{
    var a = 0;
    while(a < 3)
    {
        break;
        println("BROKEN: IMMEDIATE BREAK");
        a++;
    }
    println("after immediate break: ", a)
}

var a = "no";
{
    while (true)
    {
        var c = 2;
        while(c < 3)
        {
            while(true)
            {
                a = "yes";
                break;
                println("BROKEN: INNER IMMEDIATE BREAK");
            }
            break;
            c++;
        }
        println(a);
        break;
    }
}

var i
var j
i = 0;
while(i<100)
{
    if(i == 56)
    {
        break
    }
    j=0
    while(j<100)
    {
        if(j == 48)
        {
            break
        }
        j++;
    }
    i++;
}

println("i=", i, ", j=", j)
