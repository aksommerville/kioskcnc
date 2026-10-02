/* BellacopiaState.js
 * Decodes the weird base64-ish saved games from Bellacopia Maleficia to something tractable.
 */
 
export class BellacopiaState {
  
  /* Public accessors.
   *********************************************************************/
  
  getFullTime() {
    // clockv[0..2] = playtime,pausetime,battletime
    return (this.clockv[0] || 0) + (this.clockv[1] || 0) + (this.clockv[2] || 0);
  }
  
  getInventory() {
    // Array of strings, and it's only whether the slot is occupied; quantity isn't considered.
    // If there's an item we don't recognize, we drop it.
    const dst = [];
    for (let i=0; i<this.invstorev.length; i+=3) {
      switch (this.invstorev[i]) {
        case 1: dst.push("Stick"); break;
        case 2: dst.push("Broom"); break;
        case 3: dst.push("Divining Rod"); break;
        case 4: dst.push("Match"); break;
        case 5: dst.push("Wand"); break;
        case 6: dst.push("Fishpole"); break;
        case 7: dst.push("Bug Spray"); break;
        case 8: dst.push("Potion"); break;
        case 9: dst.push("Hookshot"); break;
        case 10: dst.push("Candy"); break;
        case 11: dst.push("Magnifier"); break;
        case 12: dst.push("Vanishing Cream"); break;
        case 13: dst.push("Compass"); break;
        case 20: dst.push("Bell"); break;
        case 22: dst.push("Barrel Hat"); break;
        case 23: dst.push("Telescope"); break;
        case 24: dst.push("Shovel"); break;
        case 25: dst.push("Pepper"); break;
        case 27: dst.push("Blue Captain's Letter"); break;
        case 28: dst.push("Blue Captain's Letter (Revised)"); break;
        case 29: dst.push("Red Captain's Letter"); break;
        case 30: dst.push("Red Captain's Letter (Revised)"); break;
        case 31: dst.push("Bomb"); break;
        case 32: dst.push("Stopwatch"); break;
        case 33: dst.push("Portable Bus Stop"); break;
        case 34: dst.push("Snowglobe"); break;
        case 35: dst.push("Tape Measure"); break;
        case 36: dst.push("Phonograph"); break;
        case 37: dst.push("Crystal Ball"); break;
        case 38: dst.push("Power Glove"); break;
        case 39: dst.push("Marionette"); break;
      }
    }
    return dst;
  }
  
  getRootDevils() {
    // Array of string, formatted for display. Tho I don't expect the UI to actually show it, probably "a root devil!" is better.
    const dst = [];
    if (this.fldv[3]) dst.push("Meadow");
    if (this.fldv[4]) dst.push("Fractia");
    if (this.fldv[5]) dst.push("South Jungle");
    if (this.fldv[6]) dst.push("Temple");
    if (this.fldv[7]) dst.push("Tundra");
    if (this.fldv[8]) dst.push("Mountains");
    if (this.fldv[9]) dst.push("Desert");
    return dst;
  }
  
  countEndorsements() {
    let c = 0;
    if (this.fldv[46]) c++;
    if (this.fldv[47]) c++;
    if (this.fldv[48]) c++;
    if (this.fldv[49]) c++;
    if (this.fldv[50]) c++;
    if (this.fldv[51]) c++;
    return c;
  }
  
  /* Gory decoding details.
   *****************************************************************************/
   
  constructor(src) {
 
/* Serial format, written out to "save" in the Egg store.
 * Starts with 10 bytes for the lengths of the individual stores.
 * Each length is 2 Base64 digits, big-endianly:
 *  - fldc (bytes encoded, ie ceil(flagc/6))
 *  - fld16c (fields)
 *  - clockc (fields)
 *  - jigstorec (records)
 *  - invstorec (records). Can't go above 26, but we use 2 bytes like the others, for consistency.
 * Followed by the heaps, Base64, in the same order:
 *  - fldv: Six flags per encoded byte, little-endianly.
 *  - fld16v: Three encoded bytes each, big-endian, the 2 high bits of each must be zero.
 *  - clockv: Five encoded bytes each, big-endian, ms. Holds about 298 hours each.
 *  - jigstorev: Five encoded bytes each, split big-endianly: 11 mapid, 8 x, 8 y, 3 xform.
 *  - invstorev: Four encoded bytes each: itemid,limit,quantity. ie straight base64 of the whole (invstorev).
 * Followed by a 30-bit checksum, performed on the encoded stream.
 */
    
    /* Read TOC.
     */
    if (src.length < 10) return;
    const fldc = this.b64scalar(src.substring(0, 2)); // bytes
    const fld16c = this.b64scalar(src.substring(2, 4)); // fields
    const clockc = this.b64scalar(src.substring(4, 6)); // fields
    const jigstorec = this.b64scalar(src.substring(6, 8)); // records
    const invstorec = this.b64scalar(src.substring(8, 10)); // records
    const reqlen = 10 + fldc + fld16c * 3 + clockc * 5 + jigstorec * 5 + invstorec * 4 + 5;
    if (src.length < reqlen) return;
    
    /* Allocate buffers.
     */
    this.fldv = new Uint8Array(fldc * 8); // One entry per field, values only zero or one.
    this.fld16v = new Uint16Array(fld16c);
    this.clockv = new Uint32Array(clockc); // ms
    this.jigstorev = new Uint32Array(jigstorec); // (mapid(11)<<19)|(x(8)<<11)|(y(8)<<3)|(xform(3)
    this.invstorev = new Uint8Array(invstorec * 3); // itemid,limit,quantity,...
    
    /* Read fldv.
     */
    let srcp = 10, dstp=0;
    for (let i=fldc; i-->0; srcp++) {
      let v = this.b64scalar(src[srcp]);
      for (let ii=6; ii-->0; v>>=1, dstp++) {
        if (v & 1) this.fldv[dstp] = 1;
      }
    }
    
    /* fld16v
     */
    dstp = 0;
    for (let i=fld16c; i-->0; srcp+=3, dstp++) {
      this.fld16v[dstp] = this.b64scalar(src.substring(srcp, srcp+3));
    }
    
    /* clockv
     */
    dstp = 0;
    for (let i=clockc; i-->0; srcp+=5, dstp++) {
      this.clockv[dstp] = this.b64scalar(src.substring(srcp, srcp+5));
    }
    
    /* jigstorev
     */
    dstp = 0;
    for (let i=jigstorec; i-->0; srcp+=5, dstp++) {
      this.jigstorev[dstp] = this.b64scalar(src.substring(srcp, srcp+5));
    }
    
    /* invstorev
     */
    dstp = 0;
    for (let i=invstorec; i-->0; srcp+=4) {
      const v = this.b64scalar(src.substring(srcp, srcp+4));
      this.invstorev[dstp++] = v >> 16;
      this.invstorev[dstp++] = v >> 8;
      this.invstorev[dstp++] = v;
    }
    
    // Then there's a 30-bit checksum, but I'm not worried about it.
  }
  
  b64scalar(src) {
    let dst = 0;
    for (let i=0; i<src.length; i++) {
      dst <<= 6;
      const ch = src.charCodeAt(i);
           if ((ch >= 0x41) && (ch <= 0x5a)) dst |= ch - 0x41;
      else if ((ch >= 0x61) && (ch <= 0x7a)) dst |= ch - 0x61 + 26;
      else if ((ch >= 0x30) && (ch <= 0x39)) dst |= ch - 0x30 + 52;
      else if (ch === 0x2b) dst |= 62;
      else if (ch === 0x2f) dst |= 63;
      else throw new Error(`Unexpected byte ${ch} in base64.`);
    }
    return dst;
  }
}
