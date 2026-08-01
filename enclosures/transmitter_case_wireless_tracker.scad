// ==========================================================================
// ROCKET TRACKER - TRANSMITTER CASE (Heltec Wireless Tracker V2 alternative)
// ==========================================================================
// Holds: Heltec Wireless Tracker V2 (one board - ESP32-S3 MCU, SX1262 LoRa
// radio + RF front-end amplifier, UC6580 GNSS, and TFT display all
// integrated, no stacking), a 3.7V 300mAh LiPo (generic "302040" size), a
// hole for the LoRa antenna to exit, and a keyed pocket that holds an
// SS12D00-G4 slide switch spliced into the battery lead. This is the
// companion case for rocket_transmitter_wireless_tracker.ino - see
// enclosures/transmitter_case.scad (Feather M0) or
// enclosures/transmitter_case_cubecell.scad (CubeCell GPS, legacy) for the
// other two transmitter builds in this repo.
//
// WHY THIS BOARD INSTEAD OF CUBECELL: CubeCell GPS is on Heltec's own
// "Phaseout" list, and its onboard GPS antenna has multiple independent
// weak/dead-on-arrival reports. Wireless Tracker V2 uses a different,
// better-regarded GNSS chip (UC6580) and is an actively-sold, actively-
// supported product - see SETUP_README.md for the full comparison.
//
// GNSS ANTENNA - NO HOLE NEEDED FOR IT
// Unlike the CubeCell case, this one only has ONE antenna hole, for LoRa.
// This board's GNSS antenna is a built-in LDS (laser-direct-structuring)
// antenna etched right onto the board - no external antenna or hole needed
// for normal use. If you ever do the resistor-rework Heltec documents for
// switching to an external GNSS antenna, you'd need to add a second hole
// yourself - not included here since it's not needed by default.
//
// WHERE THE LORA ANTENNA EXITS
// Cut through the TOP of the LID, same reasoning as the CubeCell case:
// Heltec doesn't publish a precise connector position, and the included
// antenna has some flex in its stub, so a generous, centered top hole
// avoids needing to guess an exact side-wall position that might miss.
// Nudge `ant_hole_x_frac` / `ant_hole_y_frac` below if your test-fit shows
// the connector is noticeably off-center.
//
// HOW TO USE THIS FILE
//   1. Open in OpenSCAD (openscad.org - free, all platforms).
//   2. Set the `part` variable below to "base" or "lid", press F6 (render),
//      then File > Export > Export as STL. Do this once for each part.
//   3. Leaving `part` set to "both" just previews them side by side - don't
//      export an STL in that mode.
//
// IMPORTANT - DIMENSIONS ARE BEST-EFFORT, NOT MEASURED FROM YOUR HARDWARE
//   The board envelope (53.0 x 25.4 x 10.3mm) is the larger of two
//   slightly-different numbers Heltec publishes for this board across
//   their store page and docs wiki - taken conservatively so the pocket
//   errs on the roomy side. Print the base first, test-fit before printing
//   the lid.
//
// WHERE THE SWITCH LIVES
//   Same keyed-pocket switch mount as the CubeCell case, same SS12D00-G4
//   part, same reasoning - see that file's header comment if you want the
//   full explanation. Quick summary: a narrow outer slot for the slide
//   lever only, a wider inner pocket for the switch body (captured, can't
//   pull through the narrow slot), and two small retention bumps that snap
//   the body in place with a "click." Centered on the battery bay's front
//   wall, away from the tie tab.
// ==========================================================================

part = "both"; // "base", "lid", or "both" (preview only)

// ---------------------------------------------------------------------
// Heltec Wireless Tracker V2 board - one-piece board, no stacking
// ---------------------------------------------------------------------
board_l   = 53.0;  // larger of Heltec's two published lengths (52.0/53.0)
board_w   = 25.4;  // Heltec spec: overall board width
board_h   = 10.3;  // larger of Heltec's two published heights (9.37/10.26),
                    // including the TFT glass, USB-C connector, and the
                    // shielded LoRa/GNSS chip cans
board_fit = 0.6;    // clearance added around the board footprint - snug
                     // hand-fit, not a press-fit, fine for a glued-shut case
board_vert_clear = 1.5; // headroom above the board's tallest point - a bit
                         // more than the CubeCell case's, since this board
                         // has more raised hardware (USB-C, shield cans,
                         // TFT) and its exact height breakdown isn't published

pocket_h = board_h + board_vert_clear;

// No verified mounting-hole spec is published for this board, so - same as
// both other cases in this repo - this design skips screw-post nubs and
// relies on a snug pocket fit in X/Y and the lid's underside in Z. Add your
// own posts here if you've measured your actual board's mounting holes.

// ---------------------------------------------------------------------
// Battery bay - sized for a 3.7V 300mAh LiPo, generic "302040" size code
// (3.0mm thick x 20mm wide x 40mm long). Same battery and same JST-SH
// 1.25mm pitch connector as the CubeCell build - see that file's comment
// for sourcing notes.
// ---------------------------------------------------------------------
batt_l = 40.0;
batt_w = 20.0;
batt_t = 3.0;
batt_fit = 0.8;

// ---------------------------------------------------------------------
// Case shell
// ---------------------------------------------------------------------
wall   = 2.5;
floor  = 2.0;
lid_t  = 2.0;
lip_h  = 3.0;
lip_gap = 0.3;

// Rounded corners - see the CubeCell case's comment for the full reasoning
// (comfort + helps sliding into a round airframe tube). Carried over here
// from day one instead of being a later retrofit.
case_corner_r = 3.0;

bump_r        = 1.5;
bump_protrude = 0.5;   // net interference tuned the same way as the
                        // CubeCell case's lid bumps - see that file's
                        // comment for why 0.5 instead of a smaller value
bump_groove_r     = 1.1;
bump_groove_depth = 0.65;
bump_groove_inset = bump_groove_r - bump_groove_depth;

divider_w = 2.0;
divider_gap_h = 5.0; // gap above the divider for the battery's JST wire to
                      // cross over

// ---------------------------------------------------------------------
// USB-C cutout - generously sized (same envelope as the CubeCell case's
// micro-USB cutout, which comfortably fits USB-C's slightly larger
// connector body too).
// ---------------------------------------------------------------------
usb_w = 10.0;
usb_h = 5.0;
usb_center_bias = 0.0; // mm to shift the cutout off-center, + = toward the
                        // battery/divider end - nudge if your board's port
                        // isn't centered on the board width

// ---------------------------------------------------------------------
// LoRa antenna exit hole - see "WHERE THE LORA ANTENNA EXITS" at the top
// of this file. Cut through the LID (top face), not a side wall.
// ---------------------------------------------------------------------
ant_hole_d = 8.0;
ant_hole_x_frac = 0.6; // 0 = USB end, 1 = battery/divider end
ant_hole_y_frac = 0.5; // 0.5 = centered across the board's width

// ---------------------------------------------------------------------
// Slide switch mount - SS12D00-G4, spliced inline into the battery's JST
// lead. Identical part and pocket geometry to the CubeCell case - see that
// file's header comment for the full "why" and the manufacturer-drawing
// dimensions this is built from.
// ---------------------------------------------------------------------
sw_body_l  = 8.7;
sw_body_w  = 3.7;
sw_body_h  = 3.7;
sw_lever_w = 1.5;
sw_travel  = 3.5;
sw_fit     = 0.3;
sw_web     = 1.0;

sw_slot_l   = sw_lever_w + sw_travel + sw_fit * 2;
sw_slot_h   = sw_lever_w + sw_fit * 2;
sw_pocket_l = sw_body_l + sw_fit * 2;
sw_pocket_h = sw_body_h + sw_fit * 2;
sw_pocket_depth = wall - sw_web;

sw_bump_r        = 0.8;
sw_bump_protrude = 0.3;
sw_bump_inset    = sw_bump_r - sw_bump_protrude;
sw_bump_y        = sw_web + 0.4;

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

usb_offset = (elec_pocket_w - usb_w) / 2 + usb_center_bias;

batt_top_z = floor + batt_t + batt_fit;

sw_mount_x = elec_bay_l + divider_w + batt_pocket_l / 2;
sw_mount_z = batt_top_z + ((case_h - lip_h) - batt_top_z) / 2;

lip_bump_z = case_h - (lip_h - lip_gap) / 2;

echo(str("Case outer footprint: ", case_l, " x ", case_w, " x ", case_h, " mm"));

// A rounded rectangular prism from (0,0,0) to (l,w,h), corner radius r -
// see the CubeCell case's comment for the full explanation of why only
// the outer surfaces are rounded, not the interior cavities.
module rounded_box(l, w, h, r) {
  hull() {
    for (x = [r, l - r])
      for (y = [r, w - r])
        translate([x, y, 0])
          cylinder(r = r, h = h, $fn = 32);
  }
}

// ==========================================================================
// Base
// ==========================================================================
module base() {
  difference() {
    union() {
      rounded_box(case_l, case_w, case_h - lip_h, case_corner_r);
      difference() {
        translate([0, 0, case_h - lip_h])
          rounded_box(case_l, case_w, lip_h, case_corner_r);
        translate([wall, wall, case_h - lip_h - 1])
          cube([case_l - wall*2, case_w - wall*2, lip_h + 2]);
      }
    }

    // snap-groove dimples - same pattern as the CubeCell case
    translate([wall - bump_groove_inset, case_w/2, lip_bump_z])
      sphere(r = bump_groove_r, $fn = 16);
    translate([case_l - wall + bump_groove_inset, case_w/2, lip_bump_z])
      sphere(r = bump_groove_r, $fn = 16);
    translate([case_l/2, wall - bump_groove_inset, lip_bump_z])
      sphere(r = bump_groove_r, $fn = 16);
    translate([case_l/2, case_w - wall + bump_groove_inset, lip_bump_z])
      sphere(r = bump_groove_r, $fn = 16);

    // hollow out electronics bay
    translate([wall, wall, floor])
      cube([elec_pocket_l, elec_pocket_w, case_h]);

    // hollow out battery bay
    translate([elec_bay_l + divider_w, wall, floor])
      cube([batt_pocket_l, batt_pocket_w, case_h]);

    // notch above the divider for the battery's JST wire to cross over
    translate([elec_bay_l, wall + elec_pocket_w/2 - 3, floor + pocket_h - divider_gap_h])
      cube([divider_w, 6, divider_gap_h + 1]);

    // USB-C cutout, centered vertically on the board's expected height
    translate([-1, wall + usb_offset, floor + pocket_h/2 - usb_h/2])
      cube([wall + 2, usb_w, usb_h]);

    // slide switch mount, on the battery bay's front (y=0) wall
    translate([sw_mount_x - sw_pocket_l / 2,
               wall - sw_pocket_depth,
               sw_mount_z - sw_pocket_h / 2])
      cube([sw_pocket_l, sw_pocket_depth + 1, sw_pocket_h]);

    translate([sw_mount_x - sw_slot_l / 2,
               -1,
               sw_mount_z - sw_slot_h / 2])
      cube([sw_slot_l, sw_web + 1, sw_slot_h]);
  }

  // parachute/recovery tie tab - same self-correcting formula as the
  // other two cases in this repo
  tie_tab_x = case_l - wall/2 + tie_tab_r;
  translate([tie_tab_x, case_w/2, floor])
    difference() {
      cylinder(r = tie_tab_r, h = tie_tab_thick, $fn = 32);
      translate([0, 0, -1])
        cylinder(d = tie_hole_d, h = tie_tab_thick + 2, $fn = 24);
    }

  // switch retention bumps
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
      rounded_box(case_l, case_w, lid_t, case_corner_r);
      // skirt that inserts into the base's lip cavity - stays a plain
      // rectangle (unrounded), matching the base's lip cavity
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

    // LoRa antenna exit hole, straight through the top face
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
