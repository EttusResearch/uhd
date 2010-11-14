
// Top 32 bits are integer seconds, bottom 32 are clock ticks within a second

module time_compare
  (input [63:0] time_now,
   input [63:0] trigger_time,
   output now,
   output early,
   output late, 
   output too_early);
   
   wire    sec_match   = (time_now[63:32] == trigger_time[63:32]);
   wire    sec_late    = (time_now[63:32] > trigger_time[63:32]);

   wire    tick_match  = (time_now[31:0] == trigger_time[31:0]);
   wire    tick_late   = (time_now[31:0] > trigger_time[31:0]);
/*   
   assign now 	       = sec_match & tick_match;
   assign late 	       = sec_late | (sec_match & tick_late);
   assign early        = ~now & ~late;
*/

   /*
   assign now = (time_now == trigger_time);
   assign late = (time_now > trigger_time);
   assign early = (time_now < trigger_time);
   */

   // Compare fewer bits instead of 64 to speed up logic
   // Unused bits are not significant
   //     Top bit of seconds would put us in year 2038, long after
   //        the warranty has run out :)
   //     Top 5 bits of ticks are always zero for clocks less than 134MHz
   //     "late" can drop bottom few bits of ticks, and just delay signaling
   //        of late.  
   //     "now" cannot drop those bits, it needs to be exact.
   
   wire [57:0] short_now = {time_now[62:32],time_now[26:0]};
   wire [57:0] short_trig = {trigger_time[62:32],trigger_time[26:0]};

   assign now = (short_now == short_trig);
   assign late = (short_now[57:5] > short_trig[57:5]);
   assign early = (short_now < short_trig);
   
   assign too_early    = (trigger_time[63:32] > (time_now[63:32] + 4));  // Don't wait too long
   
endmodule // time_compare
