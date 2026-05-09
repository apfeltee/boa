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
        return ((((na >> 1) & 0x7fffffff) * 2) + ((na >> nb) & 1));
    }
    if (na < 0) 
    { 
        na = (na >> 1); 
        na = na & 0x7fffffff; 
        na = na | 0x40000000; 
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

const K = [
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
    0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0xFC19DC6, 0x240CA1CC,
    0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
    0xC6E00BF3, 0xD5A79147, 0x6CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
    0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
    0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
];

function core_sha256(m, l)
{
    var HASH = [
        0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
        0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
    ];
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

/*
function print(...args)
{
    for (var i = 0; i < args.length; i++)
    {
        var arg = args[i];
        var sarg = "undefined";
        if (arg != undefined) {
            sarg = arg.toString();
        }
        process.stdout.write(sarg);
    }
}
*/

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
