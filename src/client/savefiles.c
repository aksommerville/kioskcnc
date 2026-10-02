/* savefiles.c
 * Registry of all the saved games we monitor.
 * It's OK for a file not to exist, but its directory must.
 * Notes regarding format per file are in server/scoreboard.js.
 */

const char *savefilev[]={
  "proj/all52/out/all52-linux.save",
  "proj/apothecary/out/apothecary-linux.save",
  "proj/bellacopia/out/bellacopia-linux.save",
  "proj/cherteau/out/cherteau-linux.save",
  "proj/hummfu/out/hummfu-linux.save",
  "proj/iggle/out/iggle-linux.save",
  "proj/inversion/out/inversion-linux.save",
  "proj/kabobblin/out/kabobblin-linux.save",
  "proj/kleptomania/out/kleptomania-linux.save",
  "proj/licensetoilluse/out/licensetoilluse-linux.save",
  "proj/penance/out/penance-linux.save",
  "proj/presto/out/presto-linux.save",
  "proj/queenofclocks/out/queenofclocks-linux.save",
  "proj/samsam/out/samsam-linux.save",
  "proj/upsy-downsy/out/upsy-downsy-linux.save", // NB name collapses to "upsy".
  "proj/vexularg/out/vexularg-linux.save",
  "proj/zerosigma/out/wicked-garden-linux.save", // NB name collapses to "wicked".
  "proj/wishbone/out/wishbone-linux.save",
  "proj/xrm/out/xrm-linux.save",
  "proj/younap/out/younap-linux.save",
  "proj/zennoniwa/out/zennoniwa-linux.save",
0};
