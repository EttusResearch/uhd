//
// Copyright 2020 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: PkgChdrData
//
// Description: This package defines the data types used to represent CHDR
// words and data samples in RFNoC as well as utilities for converting between
// them.
// 

package PkgChdrData;

  // Default value for ITEM_W needs to be 64 due to a bug in Vivado 2019.1.
  class ChdrData #(int CHDR_W = 64, int ITEM_W = 64);

    // CHDR bus word (CHDR_W bits)
    typedef logic       [CHDR_W-1:0] chdr_word_t ;
    typedef chdr_word_t              chdr_word_queue_t[$];

    // The item/sample word type (user-defined width, a multiple of 8 bits)
    typedef logic  [ITEM_W-1:0] item_t ;
    typedef item_t              item_queue_t[$];


    function new();
      assert ((ITEM_W % 8 == 0) && (CHDR_W % ITEM_W == 0)) else begin
        $fatal(1, "RfnocData::new: Invalid CHDR_W and/or ITEM_W");
      end
    endfunction


    // Convert a queue of items to a queue of CHDR words
    static function chdr_word_queue_t item_to_chdr(ref item_queue_t items);
      int items_per_word = CHDR_W / ITEM_W;
      int num_chdr_words = ((items.size() + items_per_word - 1) / items_per_word);
      chdr_word_queue_t chdr_words;
      chdr_word_t word;

      for (int i = 0; i < num_chdr_words; i++) begin
        for (int j = 0; j < items_per_word; j++) begin
          word[j*ITEM_W +: ITEM_W] = items[i*items_per_word + j];
        end
        chdr_words.push_back(word);
      end
      return chdr_words;
    endfunction : item_to_chdr


    // Convert a queue of CHDR words to a queue of items. The optional
    // num_bytes argument indicates how many bytes are in the item queue that
    // you want to convert.
    static function item_queue_t chdr_to_item(
      ref   chdr_word_queue_t chdr_words, 
      input int               num_bytes = -1
    );
      int items_per_word = CHDR_W / ITEM_W;
      int bytes_left;
      item_queue_t item_words;

      if (num_bytes < 0) bytes_left = chdr_words.size() * (CHDR_W / 8);
      else bytes_left = num_bytes;

      foreach (chdr_words[i]) begin
        for (int j = 0; j < items_per_word; j++) begin
          if (bytes_left > 0) begin
            item_words.push_back(chdr_words[i][j*ITEM_W +: ITEM_W]);
            bytes_left -= (ITEM_W/8);
          end
        end
      end

      assert (bytes_left == 0) else begin
        $warning("ChdrData::chdr_to_item: num_bytes is not a multiple of items");
      end

      return item_words;
    endfunction : chdr_to_item


    // Return a string representation of the contents of the CHDR queue
    static function string chdr_string(
      ref   chdr_word_queue_t chdr_words, 
      input string            format = "%X"
    );
      string str = $sformatf("%0d-bit CHDR words (%0d words or %0d bytes):\n",
        CHDR_W, chdr_words.size(), chdr_words.size() * (CHDR_W/8));
      foreach (chdr_words[i]) begin
        str = { str, $sformatf({"%5d> ", format, "\n"}, i, chdr_words[i]) };
      end
      return str;
    endfunction : chdr_string


    // Return a string representation of the contents of the item queue
    static function string item_string(
      ref   item_queue_t items,
      input string       format = "%X"
    );
      string str = $sformatf("%0d-bit items (%0d items or %0d bytes)\n", 
        ITEM_W, items.size(), items.size() * (ITEM_W/8));
      foreach (items[i]) begin
        str = { str, $sformatf({"%5d> ", format, "\n"}, i, items[i]) };
      end
      return str;
    endfunction : item_string


    // Display a string representation of the contents of a CHDR word queue
    static function void print_chdr(
      ref   chdr_word_queue_t chdr_words,
      input string            format = "%X"
    );
      $display(chdr_string(chdr_words, format));
    endfunction : print_chdr


    // Display a string representation of the contents of an item queue
    static function void print_item(
      ref   item_queue_t items,
      input string       format = "%X"
    );
      $display(item_string(items, format));
    endfunction : print_item


    // Check if the contents of two CHDR queues is equal
    static function bit chdr_equal(
      ref chdr_word_queue_t left, 
      ref chdr_word_queue_t right
    );
      if (left.size() != right.size()) return 0;
      for (int i = 0; i < left.size(); i++) begin
        if (left[i] != right[i]) return 0;
      end
      return 1;
    endfunction : chdr_equal


    // Check if the contents of two item queues is equal
    static function bit item_equal(
      ref item_queue_t left,
      ref item_queue_t right
    );
      if (left.size() != right.size()) return 0;
      for (int i = 0; i < left.size(); i++) begin
        if (left[i] != right[i]) return 0;
      end
      return 1;
    endfunction : item_equal

  endclass : ChdrData

endpackage : PkgChdrData
