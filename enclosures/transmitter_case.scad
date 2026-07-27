// ==========================================================================
// ROCKET TRACKER - TRANSMITTER CASE
// ==========================================================================
// Holds: Adafruit Feather M0 w/ RFM95 LoRa (900MHz, product 3178) with the
// Adafruit Ultimate GPS FeatherWing (product 3133) stacked on top, a 3.7V
// 500mAh LiPo (product 1578), room for a spring/wire antenna to exit the
// case, and a panel-mount cutout for an inline power switch spliced into
// the battery lead.
//
// HOW TO USE THIS FILE
//   1. Open in OpenSCAD (openscad.org - free, all platforms).
//   2. Set the `part` variable below to "base" or "lid", press F6 (render),
//      then File > Export > Export as STL. Do this once for each part.
//   3. Leaving `part` set to "both" just previews them side by side so you
//      can see how everything lines up - don't export an STL in that mode,
//      the two parts would come out as one fused blob.
//
// IMPORTANT - DIMENSIONS ARE BEST-EFFORT, NOT MEASURED FROM YOUR HARDWARE
//   Board outline, header spacing, and connector positions below come from
//   Adafruit's published Feather specification and product pages, not from
//   calipers on your actual boards. The pocket and cutouts are sized with
//   generous clearance (see the *_fit variables) specifically so small
//   errors here don't turn into a part that doesn't fit. Even so: print the
//   base first, test-fit your actual boards and battery in it BEFORE
//   printing the lid or trimming any clearance down. Cheap, fast filament
//   (low infill, no supports needed for this design) makes a test print
//   painless compared to re-measuring everything by hand up front.
//
// WHERE THE ANTENNA EXITS
//   Adafruit's guide has you solder the antenna into the "ANT" pad on the
//   right-hand edge of the Feather (USB connector oriented at the top).
//   `ant_slot_y` below controls how far down that edge the exit slot is
//   cut - the default is a rough middle-of-the-edge guess. Check where
//   your antenna is actually soldered before printing and nudge that
//   number to match; the slot is intentionally a bit oversized so small
//   misses still work.
// ==========================================================================

part = "both"; // "base", "lid", or "both" (preview only)

// ---------------------------------------------------------------------
// Feather board stack (M0 LoRa + GPS FeatherWing)
// ---------------------------------------------------------------------
feather_l          = 50.8;  // 2.0in - Feather board length (Adafruit spec)
feather_w          = 22.9;  // 0.9in - Feather board width
board_fit          = 1.0;   // clearance added around the board footprint
pcb_t              = 1.6;   // typical PCB thickness (both boards)
header_gap         = 8.6;   // vertical gap between the two boards once the
                             // GPS wing is plugged into standard-height
                             // female headers on the M0 - also has to fit
                             // the GPS wing's coin-cell holder on its
                             // underside, which is why this isn't tighter
under_board_clear  = 2.5;   // clearance below the M0 for solder joints/pins
top_stack_clear    = 7.0;   // clearance above the GPS wing for its patch
                             // antenna (~4mm tall) plus a little headroom

stack_h = under_board_clear + pcb_t + header_gap + pcb_t + top_stack_clear;

// Feather mounting holes: Adafruit's spec says "0.1in holes at each
// corner" - taken here as 2.54mm inset from every edge, giving the
// commonly-seen 1.8in x 0.7in (45.7 x 17.8mm) hole spacing. Worth
// double-checking against your actual board; these posts are support/
// registration features, not the only thing holding the stack in place.
hole_inset  = 2.54;
post_dia    = 6.0;   // support nub diameter at each estimated hole location
                      // (solid bumps the PCB rests on - not threaded, just
                      // support; add your own screw holes here if you've
                      // verified the real hole positions on your board)

// ---------------------------------------------------------------------
// Battery bay (Adafruit 1578, 3.7V 500mAh)
// ---------------------------------------------------------------------
batt_l = 36.0;
batt_w = 29.0;
batt_t = 4.75;
batt_fit = 1.5; // clearance around the battery so it slides in easily

// ---------------------------------------------------------------------
// Case shell
// ---------------------------------------------------------------------
wall   = 4.0;   // thicker than a typical enclosure on purpose - this also
                 // needs to survive a parachute landing, and the corner
                 // mounting ears below need enough wall to sit in without
                 // poking into either pocket (see ear_d comment)
floor  = 2.0;
lid_t  = 2.0;
lip_h  = 3.0;   // depth of the base's lip that the lid seats into
lip_gap = 0.25; // printer-dependent slop between lid lip and base lip

// Corner mounting ears (screw the lid to the base with 4x M3 self-tapping
// screws). Centered exactly on the case's 4 outer corners so half the
// cylinder is an "ear" sticking out and half is structurally fused into
// the case wall - ear_d's radius is kept smaller than `wall` so it can't
// poke into either pocket.
ear_d       = 6.5;
ear_pilot_d = 2.6; // pilot hole in the base (self-tapping M3)
ear_clear_d = 3.4; // clearance hole in the lid (screw passes through)

// Internal partition between the electronics bay and the battery bay -
// the battery's JST wire just runs over the top of it, no hole needed.
divider_w = 2.0;
divider_gap_h = 6.0; // leave a gap above the divider for the JST wire

// ---------------------------------------------------------------------
// USB cutout (micro USB on the Feather M0, top/short edge of the board)
// ---------------------------------------------------------------------
usb_w = 10.0;
usb_h = 5.0;
usb_offset = 6.0; // distance from the electronics-bay side wall to the
                   // near edge of the cutout - Feather's USB connector
                   // sits close to the left edge of the board; nudge this
                   // to match your board if it's off

// ---------------------------------------------------------------------
// Antenna exit slot (right-hand edge of the Feather, per Adafruit's guide)
// ---------------------------------------------------------------------
ant_slot_w = 8.0;
ant_slot_h = 4.0;
ant_slot_y = 0.45; // fraction along that edge's length - 0 = USB end,
                    // 1 = far end. 0.45 is a rough "somewhere in the
                    // middle" guess - adjust to match your board.

// ---------------------------------------------------------------------
// Inline power switch cutout (panel-mount slide switch spliced into the
// battery's JST lead, e.g. a small SPDT slide switch - adjust sw_l/sw_w
// to whatever you actually bought; a mini round toggle switch also works
// if you swap the cutout() module below for a circle())
// ---------------------------------------------------------------------
sw_l = 13.0;
sw_w = 6.5;
sw_wall_offset = 10.0; // distance from the battery bay's outer end wall

// ==========================================================================
// Derived layout
// ==========================================================================
elec_pocket_l = feather_l + board_fit * 2;
elec_pocket_w = feather_w + board_fit * 2;
// elec_bay_l is the X coordinate where the divider wall starts - just the
// near-side outer wall plus the electronics pocket. The divider itself
// (below) is the ONLY wall between the two bays; there's no separate,
// redundant far wall for the electronics bay.
elec_bay_l    = wall + elec_pocket_l;

batt_pocket_l = batt_l + batt_fit * 2;
batt_pocket_w = batt_w + batt_fit * 2;
batt_bay_l    = batt_pocket_l + wall; // battery pocket + its own far wall

case_l = elec_bay_l + divider_w + batt_bay_l;
case_w = max(elec_pocket_w, batt_pocket_w) + wall * 2;
case_h = floor + max(stack_h, batt_t + batt_fit) + lip_h;

// vertical center for the inline-switch cutout, within the usable internal
// height (below the lid's seating lip)
sw_z0 = floor + (case_h - lip_h - floor - sw_l) / 2;

echo(str("Case outer footprint: ", case_l, " x ", case_w, " x ", case_h, " mm"));

// ==========================================================================
// Base
// ==========================================================================
module base() {
  difference() {
    union() {
      // outer shell
      cube([case_l, case_w, case_h - lip_h]);
      // lip that the lid seats onto
      difference() {
        translate([wall/2, wall/2, case_h - lip_h])
          cube([case_l - wall, case_w - wall, lip_h]);
        translate([wall/2 + lip_gap, wall/2 + lip_gap, case_h - lip_h])
          cube([case_l - wall - lip_gap*2, case_w - wall - lip_gap*2, lip_h + 1]);
      }
    }

    // hollow out electronics bay
    translate([wall, wall, floor])
      cube([elec_pocket_l, elec_pocket_w, case_h]);

    // hollow out battery bay
    translate([elec_bay_l + divider_w, wall, floor])
      cube([batt_pocket_l, batt_pocket_w, case_h]);

    // notch above the divider for the battery's JST wire to cross over
    translate([elec_bay_l, wall + elec_pocket_w/2 - 3, floor + stack_h - divider_gap_h])
      cube([divider_w, 6, divider_gap_h + 1]);

    // USB cutout, centered vertically on the M0 board's expected height
    translate([-1, wall + usb_offset, floor + under_board_clear + pcb_t/2 - usb_h/2])
      cube([wall + 2, usb_w, usb_h]);

    // antenna exit slot on the "right" long edge of the electronics bay
    translate([wall + elec_pocket_l * ant_slot_y - ant_slot_w/2,
               wall + elec_pocket_w - 1,
               floor + under_board_clear + pcb_t + header_gap/2 - ant_slot_h/2])
      cube([ant_slot_w, wall + 2, ant_slot_h]);

    // inline switch cutout on the battery bay's outer end (far) wall -
    // straight through-cut, no need to rotate since this wall already
    // faces along X
    translate([case_l - wall - 1,
               wall + (batt_pocket_w - sw_w) / 2,
               sw_z0])
      cube([wall + 2, sw_w, sw_l]);
  }

  // support posts under the electronics-bay corners, roughly matching
  // the Feather's mounting-hole spacing (see hole_inset comment above)
  for (x = [hole_inset, feather_l - hole_inset])
    for (y = [hole_inset, feather_w - hole_inset])
      translate([wall + board_fit + x, wall + board_fit + y, floor])
        cylinder(d = post_dia, h = under_board_clear, $fn = 24);

  // corner mounting ears with a self-tapping screw pilot hole, spanning
  // the full height of the base shell (up to where the lid's lip seats)
  for (c = [[0, 0], [case_l, 0], [0, case_w], [case_l, case_w]])
    translate([c[0], c[1], 0])
      difference() {
        cylinder(d = ear_d, h = case_h - lip_h, $fn = 32);
        translate([0, 0, -1]) cylinder(d = ear_pilot_d, h = case_h - lip_h + 2, $fn = 16);
      }
}

// ==========================================================================
// Lid
// ==========================================================================
module lid() {
  difference() {
    union() {
      cube([case_l, case_w, lid_t]);
      // skirt that overlaps the base's lip
      translate([wall/2 + lip_gap, wall/2 + lip_gap, lid_t])
        cube([case_l - wall - lip_gap*2, case_w - wall - lip_gap*2, lip_h - lip_gap]);
      // corner ears matching the base's, so the screw heads have somewhere
      // flat to sit
      for (c = [[0, 0], [case_l, 0], [0, case_w], [case_l, case_w]])
        translate([c[0], c[1], 0])
          cylinder(d = ear_d, h = lid_t, $fn = 32);
    }
    // screw clearance holes matching the base's pilot holes
    for (c = [[0, 0], [case_l, 0], [0, case_w], [case_l, case_w]])
      translate([c[0], c[1], -1])
        cylinder(d = ear_clear_d, h = lid_t + 2, $fn = 16);
  }
}

// ==========================================================================
// Render selection
// ==========================================================================
if (part == "base") {
  base();
} else if (part == "lid") {
  lid();
} else {
  base();
  translate([0, case_w + 10, 0]) lid();
}
