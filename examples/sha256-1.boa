/**
 * Secure Hash Algorithm (SHA256)
 * http://www.webtoolkit.info/
 * Original code by Angel Marin, Paul Johnston
 **/

const chrsz = 8;

function unshiftright(a, b)
{
    var na = a;
    var nb = b;
    if(((nb > 32) || (nb == 32)) || (nb < (0-32)))
    {
        var m = (nb / 32);
        nb = nb - (m * 32);
    }
    if(nb < 0)
    {
        nb = 32 + nb;
    }
    if (nb == 0)
    {
        return ((((na >> 1) & 2147483647) * 2) + ((na >> nb) & 1));
    }
    if (na < 0) 
    { 
        na = (na >> 1); 
        na = na & 2147483647; 
        na = na | 1073741824; 
        na = (na >> (nb - 1)); 
    }
    else
    {
        na = (na >> nb); 
    }
    return na; 
}

function safe_add(x, y)
{
    var lsw = (x & 0xFFFF) + (y & 0xFFFF);
    var msw = ((x >> 16) + (y >> 16)) + (lsw >> 16);
    return (msw << 16) | (lsw & 0xFFFF);
}

function S(X, n)
{
    return (unshiftright(X, n)) | (X << (32 - n));
}

function R(X, n)
{
    return (unshiftright(X, n));
}

function Ch(x, y, z)
{
    return ((x & y) ^ ((~x) & z));
}

function Maj(x, y, z) {
    return (((x & y) ^ (x & z)) ^ (y & z));
}

function Sigma0256(x) {
    return ((S(x, 2) ^ S(x, 13)) ^ S(x, 22));
}

function Sigma1256(x) {
    return ((S(x, 6) ^ S(x, 11)) ^ S(x, 25));
}

function Gamma0256(x) {
    return ((S(x, 7) ^ S(x, 18)) ^ R(x, 3));
}

function Gamma1256(x) {
    return ((S(x, 17) ^ S(x, 19)) ^ R(x, 10));
}

const K = [1116352408,
 1899447441,
 3049323471,
 3921009573,
 961987163,
 1508970993,
 2453635748,
 2870763221,
 3624381080,
 310598401,
 607225278,
 1426881987,
 1925078388,
 2162078206,
 2614888103,
 3248222580,
 3835390401,
 4022224774,
 264347078,
 604807628,
 770255983,
 1249150122,
 1555081692,
 1996064986,
 2554220882,
 2821834349,
 2952996808,
 3210313671,
 3336571891,
 3584528711,
 113926993,
 338241895,
 666307205,
 773529912,
 1294757372,
 1396182291,
 1695183700,
 1986661051,
 2177026350,
 2456956037,
 2730485921,
 2820302411,
 3259730800,
 3345764771,
 3516065817,
 3600352804,
 4094571909,
 275423344,
 430227734,
 506948616,
 659060556,
 883997877,
 958139571,
 1322822218,
 1537002063,
 1747873779,
 1955562222,
 2024104815,
 2227730452,
 2361852424,
 2428436474,
 2756734187,
 3204031479,
 3329325298]

function core_sha256(m, l)
{
    var HASH = [1779033703, 3144134277, 1013904242, 2773480762, 1359893119, 2600822924, 528734635, 1541459225]
    var wbuf = [
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    ];
    m[l >> 5] = (m[l >> 5] | (128 << (24 - (l % 32))));
    m[(((l + 64) >> 9) << 4) + 15] = l;
    var i = 0;
    while(i < m.length)
    {
        var a = HASH[0];
        var b = HASH[1];
        var c = HASH[2];
        var d = HASH[3];
        var e = HASH[4];
        var f = HASH[5];
        var g = HASH[6];
        var h = HASH[7];
        var j = 0;
        while(j < 64)
        {
            if (j < 16)
            {
                var val = m[j + i];
                wbuf[j] = (val || 0);
            }
            else
            {
                var ta1 = safe_add(Gamma1256(wbuf[j - 2] || 0), wbuf[j - 7] || 0);
                var ta2 = safe_add(ta1, Gamma0256(wbuf[j - 15] || 0))
                wbuf[j] = safe_add(ta2, wbuf[j - 16] || 0);
            }
            var tmp1 = safe_add(h, Sigma1256(e))
            var tmp2 = safe_add(tmp1, Ch(e, f, g))
            var tmp3 = safe_add(tmp2, K[j])
            var T1 = safe_add(tmp3, wbuf[j]);
            var T2 = safe_add(Sigma0256(a), Maj(a, b, c));
            h = g;
            g = f;
            f = e;
            e = safe_add(d, T1);
            d = c;
            c = b;
            b = a;
            a = safe_add(T1, T2);
            j++;
        }
        HASH[0] = safe_add(a, HASH[0]);
        HASH[1] = safe_add(b, HASH[1]);
        HASH[2] = safe_add(c, HASH[2]);
        HASH[3] = safe_add(d, HASH[3]);
        HASH[4] = safe_add(e, HASH[4]);
        HASH[5] = safe_add(f, HASH[5]);
        HASH[6] = safe_add(g, HASH[6]);
        HASH[7] = safe_add(h, HASH[7]);
        i += 16;
    }
    return HASH;
}

function str2binb(str)
{
    var bin = [];
    var mask = (1 << chrsz) - 1;
    var i = 0;
    while(i < (str.length * chrsz))
    {
        var idx = (i >> 5);
        if(bin[i >> 5] == null)
        {
            bin[i >> 5] = 0;
        }
        var ch = str.charCodeAt(i / chrsz);
        bin[i >> 5] |= ((ch & mask) << (24 - (i % 32)));
        i += chrsz;
    }
    return bin;
}

function binb2hex(binarray)
{
    var hex_tab = "0123456789abcdef";
    var str = [];
    var i = 0;
    while(i < (binarray.length * 4))
    {
        //var ch1 = hex_tab.charAt((binarray[i >> 2] >> ((3 - i % 4) * 8 + 4)) & 0xF);
        var ch1 = hex_tab.charAt((binarray[i >> 2] >> (((3 - (i % 4)) * 8) + 4)) & 15);
        var ch2 = hex_tab.charAt((binarray[i >> 2] >> ((3 - (i % 4)) * 8)) & 15);
        str.push(ch1, ch2);
        i++;
    }
    return str.join("");
}

function SHA256(s)
{
    return binb2hex(core_sha256(str2binb(s), s.length * chrsz));
}

const strings = [
    ["foo", "2c26b46b68ffc68ff99b453c1d30413413422d706483bfa0f98a5e886266e7ae"],
    ["bar", "fcde2b2edba56bf408601fb721fe9b5c338d10ee429ea04fae5511b68fbf8fb9"],
    ["hello world", "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9"],
    ["abcdx", "f2d58ae536dc5d52ff1c83332ea1184be67febd39eaab1c98118a6cb33b9aa2a"],
    ["long text, some spaces, blah blah", "d041252a12debb14e80f563feeee2394eafecc747eb03243ad4d6539cc1a292a"],
]
for(var i=0; i<strings.length; i++)
{
    var s = strings[i][0];
    var expect = strings[i][1];
    var h = SHA256(s);
    var isok = (h == expect)
    var pre = (isok ? "ok" : "fail")
    print(pre, ": '", s, "' => ", h)
    if(!isok)
    {
        print(" (expected '", expect, "')")
    }
    print("\n")
}
