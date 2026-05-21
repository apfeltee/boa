

function unshiftright(a, b)
{
    var na = a;
    var nb = b;
    if((nb >= 32) || (nb < -32))
    {
        m = (nb / 32);
        nb = nb - (m * 32);
    }
    if(nb < 0)
    {
        nb = 32 + nb;
    }
    if (nb == 0)
    {
        return ((na >> 1) & 0x7fffffff) * 2 + ((na >> nb) & 1);
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

// hex output format. 0 - lowercase; 1 - uppercase
var hexcase = 0;

// base-64 pad character. "=" for strict RFC compliance
var b64pad  = "";

// bits per input character. 8 - ASCII; 16 - Unicode
var chrsz   = 8;



// Add integers, wrapping at 2^32.
// This uses 16-bit operations internally to work around bugs in some JS interpreters.
function safe_add(x, y)
{
    if(x == null)
    {
        x = 0;
    }
    if(y == null)
    {
        y = 0;
    }
    var lsw = (x & 0xFFFF) + (y & 0xFFFF);
    var msw = (x >> 16) + (y >> 16) + (lsw >> 16);
    return (msw << 16) | (lsw & 0xFFFF);
}


// Perform the appropriate triplet combination function for the current iteration
function sha1_ft(t, b, c, d)
{
    if(t < 20)
    {
        return (b & c) | ((~b) & d);
    }
    if(t < 40)
    {
        return b ^ c ^ d;
    }
    if(t < 60)
    {
        return (b & c) | (b & d) | (c & d);
    }
    return b ^ c ^ d;
}


// Determine the appropriate additive constant for the current iteration 
function sha1_kt(t)
{
    if(t < 20)
    {
        return 1518500249;
    }
    if(t < 40)
    {
        return 1859775393;
    }
    if(t < 60)
    {
        return -1894007588;
    }
    return -899497514;
}

// Bitwise rotate a 32-bit number to the left.
function rol(num, cnt)
{
    return (num << cnt) | unshiftright(num, (32 - cnt));
}


// Calculate the SHA-1 of an array of big-endian words, and a bit length
function core_sha1(x, l)
{
    // append padding
    x[l >> 5] |= 0x80 << (24 - l % 32);
    x[((l + 64 >> 9) << 4) + 15] = l;
    var w = [];
    var a =  1732584193;
    var b = -271733879;
    var c = -1732584194;
    var d =  271733878;
    var e = -1009589776;
    for(var ci = 0; ci < x.length; ci = ci + 16)
    {
        var olda = a;
        var oldb = b;
        var oldc = c;
        var oldd = d;
        var olde = e;
        for(var j = 0; j < 80; j++)
        {
            if(j < 16)
            {
                w[j] = x[ci + j];
            }
            else
            {
                var wv1 = w[j - 3];
                var wv2 = w[j - 8];
                var wv3 = w[j - 14];
                var wv4 = w[j - 16];
                if(wv1 == null)
                {
                    wv1 = 0;
                }
                if(wv2 == null)
                {
                    wv2 = 0;
                }
                if(wv3 == null)
                {
                    wv3 = 0;
                }
                if(wv4 == null)
                {
                    wv4 = 0;
                }
                w[j] = rol(wv1 ^ wv2 ^ wv3 ^ wv4, 1);
            }
            var t = safe_add(safe_add(rol(a, 5), sha1_ft(j, b, c, d)), safe_add(safe_add(e, w[j]), sha1_kt(j)));
            e = d;
            d = c;
            c = rol(b, 30);
            b = a;
            a = t;
        }
        a = safe_add(a, olda);
        b = safe_add(b, oldb);
        c = safe_add(c, oldc);
        d = safe_add(d, oldd);
        e = safe_add(e, olde);
    }
    return [a, b, c, d, e];
}

// Convert an 8-bit or 16-bit string to an array of big-endian words
// In 8-bit function, characters >255 have their hi-byte silently ignored.
function str2binb(str)
{
    var bin = [];
    var mask = (1 << chrsz) - 1;
    for(var si = 0; si < str.length * chrsz; si += chrsz)
    {
        if(bin[si>>5] == null)
        {
            bin[si>>5] = 0
        }
        bin[si>>5] |= (str.charCodeAt(si / chrsz) & mask) << (24 - si%32);
    }
    return bin;
}

// Calculate the HMAC-SHA1 of a key and some data
function core_hmac_sha1(key, data)
{
    var bkey = str2binb(key);
    if(bkey.length > 16)
    {
        bkey = core_sha1(bkey, key.length * chrsz);
    }
    var ipad = []
    var opad = []
    for(var hi = 0; hi < 16; hi++) 
    {
        ipad[hi] = bkey[hi] ^ 0x36363636;
        opad[hi] = bkey[hi] ^ 0x5C5C5C5C;
    }
    var hash = core_sha1(ipad + str2binb(data), 512 + data.length * chrsz);
    return core_sha1(opad + hash, 512 + 160);
}


// Convert an array of big-endian words to a string
function binb2str(bin)
{
    var str = [];
    var mask = (1 << chrsz) - 1;
    for(var bi = 0; bi < bin.length * 32; bi += chrsz)
    {
        str.push(String.chr(unshiftright(bin[bi>>5], (24 - bi%32)) & mask));
    }
    return str.join();
}

// Convert an array of big-endian words to a hex string.
function binb2hex(binarray)
{
    var hex_tab = hexcase ? "0123456789ABCDEF" : "0123456789abcdef";
    var str = [];
    for(var xi = 0; xi < binarray.length * 4; xi++)
    {
        var c1 = hex_tab.charAt((binarray[xi>>2] >> ((3 - xi%4)*8+4)) & 0xF);
        var c2 = hex_tab.charAt((binarray[xi>>2] >> ((3 - xi%4)*8  )) & 0xF);
        str.push(c1, c2)
    }
    return str.join();
}

// Convert an array of big-endian words to a base-64 string
function binb2b64(binarray)
{
    const tab = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    var str = [];
    for(var mi = 0; mi < binarray.length * 4; mi += 3)
    {
        var triplet = ((((binarray[mi   >> 2] >> 8 * (3 -  mi    % 4))  & 0xFF) << 16) | (((binarray[mi+1 >> 2] >> 8 * (3 - (mi + 1) % 4))  & 0xFF) << 8 ) |  (((binarray[mi+2 >> 2] >> 8 * (3 - (mi + 2) % 4))) & 0xFF));
        for(var ji = 0; ji < 4; ji++)
        {
            if((mi * 8 + ji * 6) > (binarray.length * 32))
            {
                str.push(b64pad);
            }
            else
            {
                str.push(tab.charAt((triplet >> 6*(3-ji)) & 0x3F));
            }
        }
    }
    return str.join();
}


// These are the functions you'll usually want to call
// They take string arguments and return either hex or base-64 encoded strings
function hex_sha1(s)
{
    return binb2hex(core_sha1(str2binb(s),s.length * chrsz));
}

function b64_sha1(s)
{
    return binb2b64(core_sha1(str2binb(s),s.length * chrsz));
}

function str_sha1(s)
{
    return binb2str(core_sha1(str2binb(s),s.length * chrsz));
}

function hex_hmac_sha1(key, data)
{
    return binb2hex(core_hmac_sha1(key, data));
}

function b64_hmac_sha1(key, data)
{
    return binb2b64(core_hmac_sha1(key, data));
}

function str_hmac_sha1(key, data)
{
    return binb2str(core_hmac_sha1(key, data));
}

var demo = [
    ["foo", "0beec7b5ea3f0fdbc95d0dd47f3c5bc275da8a33"],
    ["bar", "62cdb7020ff920e5aa642c3d4066950dd1f01f4d"],
    ["hello world", "2aae6c35c94fcfb415dbe95f408b9ce91ee846ed"],
    ["abcdx", "a96e144dfdc6380c8a4ae43bea1c81cb01215020"],
    ["long text, some spaces, blah blah", "fc0aa2379ecce86eac0dc50d921ab2cef7030d12"]
];

for(var idx=0; idx<demo.length; idx++)
{
    var itm = demo[idx];
    var k = itm[0];
    var v = itm[1];
    var ma = hex_sha1(k);
    //ma = "blah"
    var m = ma;
    //println("-- (", k, ") [", v, "] = (", ma.length, ") ", ma)
    var okstr = "FAIL";
    if(m == v)
    {
        okstr = "OK  ";
    }
    println(okstr, ": \"",k, "\" => ", m, "");

}
