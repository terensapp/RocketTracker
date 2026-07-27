// ==========================================================================
// ROCKET TRACKER - TRANSMITTER CASE
// ==========================================================================
// Holds: Adafruit Feather M0 w/ RFM95 LoRa (900MHz, product 3178) with the
// Adafruit Ultimate GPS FeatherWing (product 3133) stacked on top, a 3.7V
// 300mAh LiPo (generic "302040" size, ~40 x 20 x 3mm - see battery bay
// section below), a hole for a spring antenna to poke through, and a
// panel-mount cutout for an inline power switch spliced into the battery
// lead.
//
// CLOSURE: the lid press-fits into a lip on the base, with 4 small snap
// bumps (see bump_r/bump_protrude) that give it a bit of a "click" and
// hold it shut for handling - it's meant to be glued the rest of the way
// once you've test-fit everything, not screwed. No screw bosses sticking
// out means a noticeably smaller footprint, which matters if this needs
// to fit down a rocket's airframe.
//
// RECOVERY: there's a molded tie tab on the battery-bay end with a round
// hole through it (see tie_hole_d) for a loop of paracord or shock cord,
// so the transmitter stays attached to the parachute/harness instead of
// becoming its own separate ballistic object on the way down.
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
//   right-hand long edge of the Feather (USB connector oriented at the
//   top), positioned toward the end of the board opposite USB (that's
//   where the RFM95 radio module itself sits). The exit hole below is on
//   that same long edge, close to the far end - NOT on the far end wall
//   itself, because that wall is shared with the battery bay right next
//   to it, and the antenna's lead (about 17.5mm total) isn't long enough
//   to reach all the way across the battery bay to the case's true far
//   end. `ant_hole_y` controls exactly how close to the end it sits -
//   check where your antenna is actually soldered before printing.
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
// Battery bay - sized for a 3.7V 300mAh LiPo, generic "302040" size code
// (a standard, widely-sold LiPo footprint - the digits are the physical
// dimensions: 3.0mm thick x 20mm wide x 40mm long). Not an Adafruit part
// number - these are commonly sold on Amazon/AliExpress with a JST-PH
// connector, search "302040 300mAh lipo". Swap batt_l/batt_w/batt_t below
// if your actual battery's printed dimensions differ.
// ---------------------------------------------------------------------
batt_l = 40.0;
batt_w = 20.0;
batt_t = 3.0;
batt_fit = 1.2; // clearance around the battery so it slides in easily

// ---------------------------------------------------------------------
// Case shell
// ---------------------------------------------------------------------
wall   = 2.5;   // a bit thicker than a typical small enclosure - this has
                 // to survive a parachute landing - but slim enough to
                 // help the whole thing fit down an airframe
floor  = 2.0;
lid_t  = 2.0;
lip_h  = 3.0;    // depth of the base's lip that the lid seats into
lip_gap = 0.3;   // general clearance between lid skirt and base lip, sized
                 // for an easy push-together fit - the snap bumps below
                 // are what actually give it resistance/a "click", not a
                 // tight fit across the whole perimeter (which is harder
                 // to guarantee across different printers)

// Small interference bumps on the lid's skirt (one per side, at the
// midpoint) that the base's lip has to flex slightly to accept - gives a
// "snap" you can feel going on, without relying on the whole perimeter
// being a precise press-fit. This is meant to hold the lid on for
// handling and a test-fit, then get glued for flight - not to be a
// resealable connector.
bump_r        = 1.5;  // bump sphere radius
bump_protrude = 0.35; // how far the bump sticks out past the skirt face

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
// Antenna exit hole (right-hand long edge of the Feather, near the end
// opposite USB - see "WHERE THE ANTENNA EXITS" at the top of this file).
// Sized for Adafruit's Simple Spring Antenna - 915MHz (product 4269):
// 17.5mm long, 0.8mm wire gauge wound into a coil - the coil's outer
// diameter isn't published, so ant_hole_d below is a generous estimate
// with clearance built in, not a measured fit.
// ---------------------------------------------------------------------
ant_hole_d = 5.0;
ant_hole_y = 0.9; // fraction along that edge's length - 0 = USB end,
                   // 1 = far/divider end. 0.9 keeps it close to the far
                   // end (where the radio module is) with a little margin
                   // from the corner for wall strength.

// ---------------------------------------------------------------------
// Inline power switch cutout (panel-mount slide switch spliced into the
// battery's JST lead, e.g. a small SPDT slide switch - adjust sw_l/sw_w
// to whatever you actually bought; a mini round toggle switch also works
// if you swap the cutout() module below for a circle())
// ---------------------------------------------------------------------
sw_l = 13.0;
sw_w = 6.5;

// ---------------------------------------------------------------------
// Parachute/recovery tie tab (round hole for paracord or shock cord,
// molded onto the battery bay's outer end wall, opposite the USB/antenna
// end). Hole axis is vertical (top to bottom through the tab) so it
// prints cleanly with zero bridging or supports, regardless of print
// orientation.
// ---------------------------------------------------------------------
tie_tab_r     = 6.0;  // rounded tab radius - kept larger than `wall` on
                       // purpose (see tie_tab_x below: the tab is centered
                       // mid-wall so it always fuses into solid material
                       // no matter how these numbers get tuned, instead of
                       // relying on a hand-picked offset)
tie_tab_thick = 6.0;  // tab thickness (vertical)
tie_hole_d    = 4.5;  // fits paracord or thin shock cord with room to move

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
      // lip that the lid seats onto - a ring flush with the case's outer
      // surface, with the SAME wall thickness as the rest of the case, so
      // it's a seamless upward continuation of the walls below it (not
      // inset from the outside, which is what made the lip nearly
      // paper-thin and unable to actually hold the lid's skirt)
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
    translate([elec_bay_l, wall + elec_pocket_w/2 - 3, floor + stack_h - divider_gap_h])
      cube([divider_w, 6, divider_gap_h + 1]);

    // USB cutout, centered vertically on the M0 board's expected height
    translate([-1, wall + usb_offset, floor + under_board_clear + pcb_t/2 - usb_h/2])
      cube([wall + 2, usb_w, usb_h]);

    // antenna exit hole on the "right" long edge of the electronics bay,
    // close to the far/divider end (see "WHERE THE ANTENNA EXITS" above)
    translate([wall + elec_pocket_l * ant_hole_y,
               wall + elec_pocket_w - 1,
               floor + under_board_clear + pcb_t + header_gap/2])
      rotate([-90, 0, 0])
        cylinder(d = ant_hole_d, h = wall + 2, $fn = 24);

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

  // parachute/recovery tie tab - a single rounded lug centered mid-wall
  // (so its inner half always fuses into solid material, regardless of
  // how tie_tab_r/wall get tuned, rather than relying on a hand-picked
  // offset) with its outer half sticking out past the wall and a vertical
  // hole through it
  tie_tab_x = case_l - wall/2 + tie_tab_r;
  translate([tie_tab_x, case_w/2, floor])
    difference() {
      cylinder(r = tie_tab_r, h = tie_tab_thick, $fn = 32);
      translate([0, 0, -1])
        cylinder(d = tie_hole_d, h = tie_tab_thick + 2, $fn = 24);
    }
}

// ==========================================================================
// Lid
// ==========================================================================
module lid() {
  skirt_z_mid = lid_t + (lip_h - lip_gap) / 2;
  bump_inset  = bump_r - bump_protrude; // how far each bump center sits
                                          // inside the skirt face so only
                                          // `bump_protrude` mm pokes out

  union() {
    cube([case_l, case_w, lid_t]);
    // skirt that inserts into the base's lip cavity (which is exactly
    // `wall` thick all around, matching the rest of the case - see base())
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
