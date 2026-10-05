// find_build.js: finds what TirePressure.asi has to know about one build of SnowRunner.exe, by what the game's code
// looks like around each place, and prints it as a row for the mod's table of builds.
//   node tools\find_build.js <SnowRunner.exe, or a copy of its image from a running game> [name of the build]
// The exe as it lies on disk works when its code is plain. Steam's is wrapped by Steam's DRM, so for Steam the copy has
// to come from the running game (there the file offset is the RVA). Every value is looked for on its own, and a value
// that is not found, or found more than once, is reported as such: no row is printed then.
const fs = require('fs');

function load(file)
{
    const raw = fs.readFileSync(file);
    if (raw.readUInt16LE(0) !== 0x5A4D) throw new Error(file + ' is not a PE file');
    const pe = raw.readUInt32LE(0x3C), opt = pe + 24, count = raw.readUInt16LE(pe + 6), first = opt + raw.readUInt16LE(pe + 20);
    if (raw.readUInt16LE(opt) !== 0x20B) throw new Error(file + ' is not a 64-bit exe');
    const sizeOfImage = raw.readUInt32LE(opt + 56);
    let img = raw;
    if (raw.length < sizeOfImage)
    {
        // as on disk: lay the sections out as the loader does
        img = Buffer.alloc(sizeOfImage);
        raw.copy(img, 0, 0, raw.readUInt32LE(opt + 60));
        for (let i = 0, o = first; i < count; i++, o += 40)
        {
            const va = raw.readUInt32LE(o + 12), size = Math.min(raw.readUInt32LE(o + 8) || raw.readUInt32LE(o + 16), raw.readUInt32LE(o + 16)), at = raw.readUInt32LE(o + 20);
            if (at && size) raw.copy(img, va, at, Math.min(at + size, raw.length));
        }
    }
    const sections = [];
    for (let i = 0, o = first; i < count; i++, o += 40)
        sections.push({ name: img.toString('latin1', o, o + 8).replace(/\0+$/, ''), va: img.readUInt32LE(o + 12), size: img.readUInt32LE(o + 8), code: !!(img.readUInt32LE(o + 36) & 0x20000000) });
    const dir = (n) => ({ rva: img.readUInt32LE(opt + 112 + n * 8), size: img.readUInt32LE(opt + 112 + n * 8 + 4) });
    return { img, mapped: img !== raw, stamp: img.readUInt32LE(pe + 8), base: img.readBigUInt64LE(opt + 24), sizeOfImage, sections, imports: dir(1), pdata: dir(3) };
}

const hex = (n) => '0x' + n.toString(16);

// every place in the code sections where the bytes are those of the pattern ("48 8b ?? e8", ?? = any byte)
function find(pe, pattern, from, to)
{
    const p = pattern.split(/\s+/).map((x) => x === '??' ? -1 : parseInt(x, 16)), hits = [];
    const lead = p.findIndex((x) => x >= 0);
    for (const s of pe.sections)
    {
        if (!s.code) continue;
        const begin = Math.max(s.va, from === undefined ? 0 : from), end = Math.min(s.va + s.size, to === undefined ? pe.img.length : to) - p.length;
        for (let o = begin; o <= end; o++)
        {
            o = pe.img.indexOf(p[lead], o + lead) - lead;
            if (o < begin || o > end) break;
            let k = 0;
            while (k < p.length && (p[k] < 0 || pe.img[o + k] === p[k])) k++;
            if (k === p.length) hits.push(o);
        }
    }
    return hits;
}

// the function a code address lies in, with what its unwind data says about its stack: pushes and bytes of locals
function functionOf(pe, rva)
{
    const img = pe.img;
    for (let o = pe.pdata.rva; o + 12 <= pe.pdata.rva + pe.pdata.size; o += 12)
    {
        const begin = img.readUInt32LE(o), end = img.readUInt32LE(o + 4);
        if (!(begin <= rva && rva < end)) continue;
        const u = img.readUInt32LE(o + 8), codes = img[u + 2];
        let pushes = 0, alloc = 0;
        for (let i = 0; i < codes; i++)
        {
            const op = img[u + 5 + i * 2] & 15, info = img[u + 5 + i * 2] >> 4;
            if (op === 0) pushes++;
            else if (op === 1) { alloc += info ? img.readUInt32LE(u + 6 + i * 2) : img.readUInt16LE(u + 6 + i * 2) * 8; i += info ? 2 : 1; }
            else if (op === 2) alloc += info * 8 + 8;
            else if (op === 4 || op === 8) i += 1;
            else if (op === 5 || op === 9) i += 2;
        }
        return { begin, end, pushes, alloc, chained: !!((img[u] >> 3) & 4) };
    }
    return null;
}

// the vtable of a class, by the name its type information carries (".?AVhkpCylinderShape@@")
function vtableOf(pe, name, notes)
{
    const img = pe.img, text = Buffer.from(name + '\0', 'latin1'), found = [];
    for (let s = img.indexOf(text); s >= 0; s = img.indexOf(text, s + 1))
    {
        const type = s - 16;
        // the object locator of the class's first vtable: signature 1, offset 0, the type, and its own RVA
        for (let o = 0; o + 24 <= img.length; o += 4)
        {
            if (img.readUInt32LE(o + 12) !== type || img.readUInt32LE(o) !== 1 || img.readUInt32LE(o + 4) !== 0 || img.readUInt32LE(o + 20) !== o) continue;
            const pointer = Buffer.alloc(8);
            pointer.writeBigUInt64LE(pe.base + BigInt(o));
            for (let v = img.indexOf(pointer); v >= 0; v = img.indexOf(pointer, v + 1)) if (v % 8 === 0) found.push(v + 8);
        }
    }
    if (found.length !== 1) notes.push('the vtable of ' + name + ': ' + found.length + ' found ' + found.map(hex).join(' '));
    return found.length === 1 ? found[0] : 0;
}

// the import slot of a function
function importSlot(pe, dllName, functionName)
{
    const img = pe.img;
    for (let d = pe.imports.rva; d && img.readUInt32LE(d + 12); d += 20)
    {
        const n = img.readUInt32LE(d + 12), dll = img.toString('latin1', n, img.indexOf(0, n));
        if (dll.toLowerCase() !== dllName.toLowerCase()) continue;
        const names = img.readUInt32LE(d) || img.readUInt32LE(d + 16), slots = img.readUInt32LE(d + 16);
        for (let k = 0; img.readBigUInt64LE(names + k * 8); k++)
        {
            const v = img.readBigUInt64LE(names + k * 8);
            if (v >> 63n) continue;
            const h = Number(v) + 2;
            if (img.toString('latin1', h, img.indexOf(0, h)) === functionName) return slots + k * 8;
        }
    }
    return 0;
}

const [file, buildName = 'this build'] = process.argv.slice(2);
if (!file) { console.log('usage: node tools\\find_build.js <SnowRunner.exe or a copy of its image> [name of the build]'); process.exit(2); }
const pe = load(file), img = pe.img, notes = [];
console.log(file + ': build stamp ' + hex(pe.stamp) + ', image ' + hex(pe.sizeOfImage) + ' bytes, ' + (pe.mapped ? 'read as it lies on disk' : 'a copy of the loaded image') +
            ', sections ' + pe.sections.map((s) => s.name + (s.code ? '*' : '')).join(' '));
const one = (what, hits) => { if (hits.length !== 1) notes.push(what + ': ' + hits.length + ' found ' + hits.slice(0, 8).map(hex).join(' ')); return hits.length === 1 ? hits[0] : 0; };

// the wheel's parameter object and the wheel's collision cylinder, by class name
const wheelVtable = vtableOf(pe, '.?AUTRUCK_WHEEL_PARAMS@combine@@', notes);
const cylinderVtable = vtableOf(pe, '.?AVhkpCylinderShape@@', notes);

// the game's damage update, by its first instructions (they hold the vehicle's +0x150)
const damageUpdate = one('the damage update', find(pe,
    '48 8b c4 48 89 48 08 55 56 57 41 54 41 55 41 56 41 57 48 83 ec 50 48 c7 40 98 fe ff ff ff 48 89 58 18 0f 29 70 b8 0f 29 78 a8 4c 8b e9 48 8d b9 50 01 00 00 48 8b cf e8'));

// the truck update's question to Windows which window is in front: the call through the import slot, in the function
// that keeps its first argument in its home slot
const slot = importSlot(pe, 'USER32.dll', 'GetForegroundWindow');
if (!slot) notes.push('no import of GetForegroundWindow from USER32.dll');
let truckReturn = 0, firstArgument = 0, control = 0;
const site = one('the truck update\'s call of GetForegroundWindow', find(pe, '40 b7 01 40 88 7c 24 50 44 0f b6 ff ff 15 ?? ?? ?? ?? 48 8b d8 e8 ?? ?? ?? ?? 48 3b c3 74'));
if (site)
{
    const call = site + 12, f = functionOf(pe, call);
    if (call + 6 + img.readInt32LE(call + 2) !== slot) notes.push('the call at ' + hex(call) + ' does not go through the import slot ' + hex(slot));
    else if (!f) notes.push('no function around ' + hex(call));
    else
    {
        const head = [...img.subarray(f.begin, f.begin + 33)].map((b) => b.toString(16).padStart(2, '0')).join(' ');
        if (head !== '48 8b c4 f3 0f 11 58 20 f3 0f 11 50 18 48 89 50 10 48 89 48 08 55 53 56 57 41 54 41 55 41 56 41 57')
            notes.push('the truck update at ' + hex(f.begin) + ' begins differently: ' + head);
        else if (f.chained) notes.push('the truck update\'s unwind data is chained: its stack has to be read by hand');
        else
        {
            truckReturn = call + 6;
            // from the slot of the call's return address up to the function's first argument: its locals, its pushes,
            // its own return address, then the home slot
            firstArgument = f.alloc + f.pushes * 8 + 16;
            // the truck control global: the second of three small "return the address of a global" calls in a row
            const three = one('the three globals read in the truck update', find(pe,
                'e8 ?? ?? ?? ?? 4c 8b 20 4c 89 65 c8 e8 ?? ?? ?? ?? 48 8b 30 48 89 75 a0 e8 ?? ?? ?? ?? 48 89 45 18 4c 8b 00 80 be d8 00 00 00 00', f.begin, f.end));
            if (three)
            {
                const target = three + 12 + 5 + img.readInt32LE(three + 13);
                if (img[target] === 0x48 && img[target + 1] === 0x8d && img[target + 2] === 0x05 && img[target + 7] === 0xc3) control = target + 7 + img.readInt32LE(target + 3);
                else notes.push('the call at ' + hex(three + 12) + ' goes to ' + hex(target) + ', which is not "lea rax, [global]; ret"');
            }
        }
    }
}

const row = { stamp: pe.stamp, image: pe.sizeOfImage, control, wheelVtable, cylinderVtable, damageUpdate, foregroundSlot: slot, truckUpdateReturn: truckReturn, firstArgument };
for (const k in row) console.log('  ' + k.padEnd(18) + (row[k] ? hex(row[k]) : 'NOT FOUND'));
if (notes.length || Object.values(row).some((v) => !v))
{
    for (const n of notes) console.log('  ! ' + n);
    console.log('No row: not everything was found.');
    process.exit(1);
}
console.log('Row for kBuilds in src\\tire_pressure.cpp:');
console.log('    { ' + Object.values(row).map(hex).join(', ') + ' }, // ' + buildName);
