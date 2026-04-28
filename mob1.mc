
function A(k, plusone, minusone, minusanother, plusanother, plusnull) {
    var callme;
    callme = func() {
        k = k - 1;
        return A(k, callme, plusone, minusone, minusanother, plusanother);
    };
    if (k <= 0) {
        return plusanother() + plusnull();
    } else {
        return callme();
    }
}

function vnull()  { return  0; }
function vone()  { return  1; }
function vminusone() { return -1; }

function main()
{
    var i=0;
    while(i<11)
    {
        print("", i, " -> ");
        print(A(i, vone, vminusone, vminusone, vone, vnull));
        print("\n");
        i = i + 1;
    }
}

main()

