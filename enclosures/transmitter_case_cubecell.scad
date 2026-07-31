// ==========================================================================
// ROCKET TRACKER - TRANSMITTER CASE (CubeCell GPS all-in-one alternative)
// ==========================================================================
// Holds: Heltec CubeCell GPS, HTCC-AB02S (one board - MCU, SX1262 LoRa radio,
// GPS, and OLED all integrated, no stacking), a 3.7V 300mAh LiPo (generic
// "302040" size), a hole for a u.FL/IPEX antenna pigtail to exit, and a
// keyed pocket that holds an SS12D00-G4 slide switch spliced into the
// battery lead. This is the companion case for rocket_transmitter_cubecell.ino
// - see enclosures/transmitter_case.scad instead if you're building the
// Feather M0 version (three-board stack, spring antenna soldered direct).
//
// WHY THIS IS SMALLER: the Feather M0 build stacks three boards (M0 + GPS
// FeatherWing + battery) with headroom between them for header pins and a
// patch antenna. CubeCell GPS puts the MCU, radio, and GPS on one board, so
// this case only needs to be as tall as one PCB plus its tallest connector,
// not a whole stack. Rough numbers: this case's footprint is about 35mm
// wide x 16mm tall, versus 30mm x 26mm for the Feather version - shallower,
// which matters more than raw length for sliding into a narrow airframe.
//
// CLOSURE / RECOVERY: same snap-lid-and-glue closure and parachute tie tab
// as the Feather case - see that file's header comment for the reasoning,
// not repeated here.
//
// HOW TO USE THIS FILE
//   1. Open in OpenSCAD (openscad.org - free, all platforms).
//   2. Set the `part` variable below to "base" or "lid", press F6 (render),
//      then File > Export > Export as STL. Do this once for each part.
//   3. Leaving `part` set to "both" just previews them side by side - don't
//      export an STL in that mode.
//
// IMPORTANT - DIMENSIONS ARE BEST-EFFORT, NOT MEASURED FROM YOUR HARDWARE
//   The board envelope (55.9 x 27.9 x 9.5mm) comes from Heltec's published
//   spec sheet for HTCC-AB02S, not calipers on your actual board. Print the
//   base first, test-fit before printing the lid.
//
// WHERE THE ANTENNA EXITS - READ THIS ONE CAREFULLY
//   Unlike the Feather build (a spring antenna soldered directly to a fixed
//   pad), CubeCell GPS has a u.FL/IPEX antenna connector - you'll need an
//   IPEX-pigtail antenna (see the BOM note in the README) rather than a
//   solder-in spring antenna. Heltec's pin map diagram shows roughly where
//   that connector sits, but wasn't precise enough to trust for a tight
//   side-wall cutout. So instead of guessing a side position and risking a
//   hole that misses the connector, the antenna hole below is cut through
//   the TOP of the LID, sized generously (8mm) and centered over the board
//   - the u.FL pigtail is a thin, flexible cable, so it can bend to reach an
//   opening almost anywhere above the board. Nudge `ant_hole_x_frac` /
//   `ant_hole_y_frac` below if your test-fit shows the connector is
//   noticeably off-center, but don't assume you need to before checking.
//
// WHERE THE SWITCH LIVES
//   The switch mount is a keyed pocket in the battery bay's front wall (the
//   long wall at y=0), centered along that bay's length - not on the end
//   wall the tie tab is bolted to. Earlier versions of this case put a
//   switch cutout on that end wall, right next to the tie tab, which looked
//   like a stray unexplained hole and put a hole through the same wall the
//   tie tab has to stay solid for. Moving it to the battery bay's side wall
//   fixes both problems. The pocket is two nested cuts: a narrow slot
//   through the outer face sized only for the slide lever to poke through
//   and travel in, and a wider, shallower pocket behind it (open to the
//   battery bay's interior) sized to capture the switch body. The body
//   can't pull out through the narrow outer slot, and two small retention
//   bumps (same idea as the lid's snap bumps, just smaller) protrude into
//   the pocket from its top and bottom faces - the switch body has to press
//   past them to seat, then they nest against its top/bottom face and hold
//   it with a "click", no glue required (though a dab is still fine if you
//   want extra insurance) - see the "Slide switch mount" parameters below
//   for the dimensions, taken from the SS12D00-G4
//   manufacturer drawing (body 8.7 x 3.7 x 3.7mm, lever 1.5 x 1.5 x 4mm).
//   The 3-pin through-hole legs on the underside of that switch aren't used
//   here - the switch is wired inline into the battery lead by hand, not
//   soldered to a PCB, so only the body and lever envelope matter for the
//   case. `sw_travel` is an assumed slide throw, not from the drawing (which
//   doesn't dimension it) - test-fit and adjust if your switch's actual
//   throw differs.
// ==========================================================================

part = "both"; // "base", "lid", or "both" (preview only)

// ---------------------------------------------------------------------
// CubeCell GPS board (HTCC-AB02S) - one-piece board, no stacking
// ---------------------------------------------------------------------
board_l   = 55.9;  // Heltec spec: overall board length
board_w   = 27.9;  // Heltec spec: overall board width
board_h   = 9.5;   // Heltec spec: overall board height, including the
                    // onboard OLED, connectors, and any other tallest part
board_fit = 1.0;    // clearance added around the board footprint
board_vert_clear = 1.5; // headroom above the board's tallest point

pocket_h = board_h + board_vert_clear;

// No verified mounting-hole spec is published for this board, so this
// design deliberately skips screw-post nubs (unlike the Feather case, which
// had Adafruit's documented hole spacing to work from) rather than guess
// hole positions that might be wrong. The board is held by a snug pocket
// fit in X/Y and the lid's underside in Z - fine for a glued-shut case that
// isn't opened repeatedly. Add your own posts here if you've measured your
// actual board's mounting holes.

// ---------------------------------------------------------------------
// Battery bay - sized for a 3.7V 300mAh LiPo, generic "302040" size code
// (3.0mm thick x 20mm wide x 40mm long). Same battery as the Feather build
// - see that file's comment for sourcing notes. One difference: CubeCell
// GPS's onboard battery connector is JST-SH 1.25mm pitch (2-pin), NOT the
// JST-PH 2.0mm pitch used on the Feather - double check which connector
// your battery actually ships with (see the BOM note) before ordering.
// ---------------------------------------------------------------------
batt_l = 40.0;
batt_w = 20.0;
batt_t = 3.0;
batt_fit = 1.2;

// ---------------------------------------------------------------------
// Case shell
// ---------------------------------------------------------------------
wall   = 2.5;
floor  = 2.0;
lid_t  = 2.0;
lip_h  = 3.0;
lip_gap = 0.3;

bump_r        = 1.5;
bump_protrude = 0.35;

divider_w = 2.0;
divider_gap_h = 5.0; // gap above the divider for the battery's JST wire to
                      // cross over - shorter than the Feather case's since
                      // this whole case is a lot shallower

// ---------------------------------------------------------------------
// USB cutout (micro USB, on one short edge of the board per Heltec's spec)
// ---------------------------------------------------------------------
usb_w = 10.0;
usb_h = 5.0;
usb_offset = 6.0; // distance from the electronics-bay side wall to the
                   // near edge of the cutout - a centered guess; nudge to
                   // match your board if it's off

// ---------------------------------------------------------------------
// Antenna exit hole - see "WHERE THE ANTENNA EXITS" at the top of this file.
// Cut through the LID (top face), not a side wall.
// ---------------------------------------------------------------------
ant_hole_d = 8.0;
ant_hole_x_frac = 0.6; // 0 = USB end, 1 = battery/divider end - biased
                        // slightly toward the radio side of the board
ant_hole_y_frac = 0.5; // 0.5 = centered across the board's width

// ---------------------------------------------------------------------
// Slide switch mount - SS12D00-G4, spliced inline into the battery's JST
// lead. Dimensions from the manufacturer's dimensioned drawing - see
// "WHERE THE SWITCH LIVES" above for how the pocket geometry uses them.
// ---------------------------------------------------------------------
sw_body_l  = 8.7;  // switch body length (along the slide axis)
sw_body_w  = 3.7;  // switch body depth (the dimension going into the wall)
sw_body_h  = 3.7;  // switch body height
sw_lever_w = 1.5;  // slide lever width, from the drawing
sw_travel  = 3.5;  // assumed slide throw - not on the drawing, test-fit
sw_fit     = 0.3;  // clearance added around the body pocket
sw_web     = 1.0;  // solid material left at the outer face around the
                    // lever slot, so the body pocket stays a captured,
                    // shouldered cut instead of an open hole to outside

sw_slot_l   = sw_lever_w + sw_travel + sw_fit * 2; // outer slot, lever only
sw_slot_h   = sw_lever_w + sw_fit * 2;
sw_pocket_l = sw_body_l + sw_fit * 2;               // inner pocket, body
sw_pocket_h = sw_body_h + sw_fit * 2;
sw_pocket_depth = wall - sw_web; // how far the wide pocket cuts in from
                                  // the inner wall face toward outside -
                                  // whatever's left of the body's depth
                                  // just extends into the already-hollow
                                  // battery bay, no extra cut needed there

// Retention bumps - two small nubs on the pocket's top and bottom faces,
// same idea as the lid's snap bumps below: they protrude slightly into the
// pocket cavity, so the switch body has to press past a bit of interference
// to seat, then they nest against its top/bottom face and hold it in place
// with a "click" - no glue required to keep it from rattling loose, though
// a dab is still fine if you want extra insurance.
sw_bump_r        = 0.8;
sw_bump_protrude = 0.3;
sw_bump_inset    = sw_bump_r - sw_bump_protrude;
sw_bump_y        = sw_web + 0.4; // near the shoulder, so it clicks right as
                                  // the switch bottoms out against the stop

// ---------------------------------------------------------------------
// Parachute/recovery tie tab
// ---------------------------------------------------------------------
tie_tab_r     = 6.0;
tie_tab_thick = 6.0;
tie_hole_d    = 4.5;

// ==========================================================================
// Derived layout
// ==========================================================================
elec_pocket_l = board_l + board_fit * 2;
elec_pocket_w = board_w + board_fit * 2;
elec_bay_l    = wall + elec_pocket_l;

batt_pocket_l = batt_l + batt_fit * 2;
batt_pocket_w = batt_w + batt_fit * 2;
batt_bay_l    = batt_pocket_l + wall;

case_l = elec_bay_l + divider_w + batt_bay_l;
case_w = max(elec_pocket_w, batt_pocket_w) + wall * 2;
case_h = floor + max(pocket_h, batt_t + batt_fit) + lip_h;

// switch mount position - centered along the battery bay's length, and
// centered in the wall's height between the floor and the lip
sw_mount_x = elec_bay_l + divider_w + batt_pocket_l / 2;
sw_mount_z = floor + (case_h - lip_h - floor) / 2;

echo(str("Case outer footprint: ", case_l, " x ", case_w, " x ", case_h, " mm"));

// ==========================================================================
// Base
// ==========================================================================
module base() {
  difference() {
    union() {
      cube([case_l, case_w, case_h - lip_h]);
      // lip that the lid seats onto - flush with the case's outer surface,
      // same wall thickness as the rest of the case (validated fix carried
      // over from the Feather case - see that file's changelog if curious)
      difference() {
        translate([0, 0, case_h - lip_h])
          cube([case_l, case_w, lip_h]);
        translate([wall, wall, case_h - lip_h - 1])
          cube([case_l - wall*2, case_w - wall*2, lip_h + 2]);
      }
    }

    // hollow out electronics bay
    translate([wall, wall, floor])
      cube([elec_pocket_l, elec_pocket_w, case_h]);

    // hollow out battery bay
    translate([elec_bay_l + divider_w, wall, floor])
      cube([batt_pocket_l, batt_pocket_w, case_h]);

    // notch above the divider for the battery's JST wire to cross over
    translate([elec_bay_l, wall + elec_pocket_w/2 - 3, floor + pocket_h - divider_gap_h])
      cube([divider_w, 6, divider_gap_h + 1]);

    // USB cutout, centered vertically on the board's expected height
    translate([-1, wall + usb_offset, floor + pocket_h/2 - usb_h/2])
      cube([wall + 2, usb_w, usb_h]);

    // slide switch mount, on the battery bay's front (y=0) wall - see
    // "WHERE THE SWITCH LIVES" at the top of this file. Two nested cuts:

    // (1) wide pocket for the switch body, cut from the inner wall face
    // toward outside, open to the battery bay's hollow interior so pins/
    // wires are reachable and stopping short of the outer face by sw_web
    translate([sw_mount_x - sw_pocket_l / 2,
               wall - sw_pocket_depth,
               sw_mount_z - sw_pocket_h / 2])
      cube([sw_pocket_l, sw_pocket_depth + 1, sw_pocket_h]);

    // (2) narrow slot through the remaining outer web, for the lever only -
    // narrower than the body pocket above, so the body can't pull through it
    translate([sw_mount_x - sw_slot_l / 2,
               -1,
               sw_mount_z - sw_slot_h / 2])
      cube([sw_slot_l, sw_web + 1, sw_slot_h]);
  }

  // parachute/recovery tie tab - same self-correcting formula as the
  // Feather case, guaranteeing it always fuses into solid wall material
  tie_tab_x = case_l - wall/2 + tie_tab_r;
  translate([tie_tab_x, case_w/2, floor])
    difference() {
      cylinder(r = tie_tab_r, h = tie_tab_thick, $fn = 32);
      translate([0, 0, -1])
        cylinder(d = tie_hole_d, h = tie_tab_thick + 2, $fn = 24);
    }

  // switch retention bumps - protrude into the pocket cavity cut above, so
  // the switch body has to snap past them to seat. See the parameter block
  // comment above for the reasoning.
  translate([sw_mount_x, sw_bump_y, sw_mount_z + sw_pocket_h/2 + sw_bump_inset])
    sphere(r = sw_bump_r, $fn = 16);
  translate([sw_mount_x, sw_bump_y, sw_mount_z - sw_pocket_h/2 - sw_bump_inset])
    sphere(r = sw_bump_r, $fn = 16);
}

// ==========================================================================
// Lid
// ==========================================================================
module lid() {
  skirt_z_mid = lid_t + (lip_h - lip_gap) / 2;
  bump_inset  = bump_r - bump_protrude;

  difference() {
    union() {
      cube([case_l, case_w, lid_t]);
      // skirt that inserts into the base's lip cavity
      translate([wall + lip_gap, wall + lip_gap, lid_t])
        cube([case_l - wall*2 - lip_gap*2, case_w - wall*2 - lip_gap*2, lip_h - lip_gap]);

      // snap bumps, one at the midpoint of each of the 4 skirt faces
      translate([wall + lip_gap + bump_inset, case_w/2, skirt_z_mid])
        sphere(r = bump_r, $fn = 16);
      translate([case_l - wall - lip_gap - bump_inset, case_w/2, skirt_z_mid])
        sphere(r = bump_r, $fn = 16);
      translate([case_l/2, wall + lip_gap + bump_inset, skirt_z_mid])
        sphere(r = bump_r, $fn = 16);
      translate([case_l/2, case_w - wall - lip_gap - bump_inset, skirt_z_mid])
        sphere(r = bump_r, $fn = 16);
    }

    // antenna exit hole, straight through the top face - see the header
    // comment for why this is on the lid instead of a side wall
    translate([elec_pocket_l * ant_hole_x_frac + wall,
               elec_pocket_w * ant_hole_y_frac + wall,
               -1])
      cylinder(d = ant_hole_d, h = lid_t + lip_h + 2, $fn = 24);
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
