
-- Company: ZPU4 generic memory interface CPU
-- Engineer: Øyvind Harboe

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
use IEEE.STD_LOGIC_arith.ALL;

library work;
use work.zpu_config.all;
use work.zpupkg.all;





entity zpu_core is
    Port ( clk : in std_logic;
	 		  areset : in std_logic;
	 		  enable : in std_logic; 
	 		  mem_req : out std_logic;
	 		  mem_we : out std_logic;
	 		  mem_ack : in std_logic; 
	 		  mem_read : in std_logic_vector(wordSize-1 downto 0);
	 		  mem_write : out std_logic_vector(wordSize-1 downto 0);
			  out_mem_addr : out std_logic_vector(maxAddrBitIncIO downto 0);
	 		  mem_writeMask: out std_logic_vector(wordBytes-1 downto 0);
	 		  interrupt : in std_logic;
	 		  break : out std_logic;
	 		  zpu_status : out std_logic_vector(63 downto 0));
end zpu_core;

architecture behave of zpu_core is

type InsnType is 
(
State_AddTop,
State_Dup,
State_DupStackB,
State_Pop,
State_Popdown,
State_Add,
State_Or,
State_And,
State_Store,
State_AddSP,
State_Shift,
State_Nop,
State_Im,
State_LoadSP,
State_StoreSP,
State_Emulate,
State_Load,
State_PushPC,
State_PushSP,
State_PopPC,
State_PopPCRel,
State_Not,
State_Flip,
State_PopSP,
State_Neqbranch,
State_Eq,
State_Loadb,
State_Mult,
State_Lessthan,
State_Lessthanorequal,
State_Ulessthanorequal,
State_Ulessthan,
State_Pushspadd,
State_Call,
State_Callpcrel,
State_Sub,
State_Break,
State_Storeb,
State_Interrupt,
State_InsnFetch
);

type StateType is 
(
State_Idle, -- using first state first on the list out of paranoia 
State_Load2,
State_Popped,
State_LoadSP2,
State_LoadSP3,
State_AddSP2,
State_Fetch,
State_Execute,
State_Decode,
State_Decode2,
State_Resync,

State_StoreSP2,
State_Resync2,
State_Resync3,
State_Loadb2,
State_Storeb2,
State_Mult2,
State_Mult3,
State_Mult5,
State_Mult6,
State_Mult4,
State_BinaryOpResult
); 


signal	pc				: std_logic_vector(maxAddrBitIncIO downto 0);
signal	sp				: std_logic_vector(maxAddrBitIncIO downto minAddrBit);
signal	incSp			: std_logic_vector(maxAddrBitIncIO downto minAddrBit);
signal	incIncSp			: std_logic_vector(maxAddrBitIncIO downto minAddrBit);
signal	decSp			: std_logic_vector(maxAddrBitIncIO downto minAddrBit);
signal  stackA   		: std_logic_vector(wordSize-1 downto 0);
signal  binaryOpResult		: std_logic_vector(wordSize-1 downto 0);
signal  multResult2  		: std_logic_vector(wordSize-1 downto 0);
signal  multResult3  		: std_logic_vector(wordSize-1 downto 0);
signal  multResult  		: std_logic_vector(wordSize-1 downto 0);
signal  multA 		: std_logic_vector(wordSize-1 downto 0);
signal  multB  		: std_logic_vector(wordSize-1 downto 0);
signal  stackB  		: std_logic_vector(wordSize-1 downto 0);
signal	idim_flag			: std_logic;
signal	busy 				: std_logic;
signal mem_readEnable : std_logic;
signal mem_addr : std_logic_vector(maxAddrBitIncIO downto minAddrBit);
signal mem_delayAddr : std_logic_vector(maxAddrBitIncIO downto minAddrBit);
signal mem_delayReadEnable : std_logic;
signal mem_busy : std_logic;
signal decodeWord : std_logic_vector(wordSize-1 downto 0);


signal state : StateType;
signal insn : InsnType;
type InsnArray is array(0 to wordBytes-1) of InsnType;
signal decodedOpcode : InsnArray;

type OpcodeArray is array(0 to wordBytes-1) of std_logic_vector(7 downto 0);

signal opcode : OpcodeArray;




signal	begin_inst			: std_logic;
signal trace_opcode		: std_logic_vector(7 downto 0);
signal	trace_pc				: std_logic_vector(maxAddrBitIncIO downto 0);
signal	trace_sp				: std_logic_vector(maxAddrBitIncIO downto minAddrBit);
signal	trace_topOfStack				: std_logic_vector(wordSize-1 downto 0);
signal	trace_topOfStackB				: std_logic_vector(wordSize-1 downto 0);

signal	out_mem_req : std_logic;

signal inInterrupt : std_logic;

-- state machine.

begin

	zpu_status(maxAddrBitIncIO downto 0) <= trace_pc;
	zpu_status(31) <= '1';
	zpu_status(39 downto 32) <= trace_opcode;
	zpu_status(40) <= '1' when (state = State_Idle) else '0';
	zpu_status(62) <= '1';

	traceFileGenerate:
   if Generate_Trace generate
	trace_file: trace port map (
       	clk => clk,
       	begin_inst => begin_inst,
       	pc => trace_pc,
		opcode => trace_opcode,
		sp => trace_sp,
		memA => trace_topOfStack,
		memB => trace_topOfStackB,
		busy => busy,
		intsp => (others => 'U')
        );
	end generate;

	
	-- the memory subsystem will tell us one cycle later whether or 
	-- not it is busy
	out_mem_addr(maxAddrBitIncIO downto minAddrBit) <= mem_addr;
	out_mem_addr(minAddrBit-1 downto 0) <= (others => '0');
	mem_req <= out_mem_req;
	
	incSp <= sp + 1;
	incIncSp <= sp + 2;
	decSp <= sp - 1;
	
	mem_busy <= out_mem_req and not mem_ack; -- '1' when the memory is busy

	opcodeControl:
	process(clk, areset)
		variable tOpcode : std_logic_vector(OpCode_Size-1 downto 0);
		variable spOffset : std_logic_vector(4 downto 0);
		variable tSpOffset : std_logic_vector(4 downto 0);
		variable nextPC : std_logic_vector(maxAddrBitIncIO downto 0);
		variable tNextState : InsnType;
		variable tDecodedOpcode : InsnArray;
		variable tMultResult : std_logic_vector(wordSize*2-1 downto 0);
	begin
		if areset = '1' then
			state <= State_Idle;
			break <= '0';
			sp <= spStart(maxAddrBitIncIO downto minAddrBit);
			 
			pc <= (others => '0');
			idim_flag <= '0';
			begin_inst <= '0';
			mem_we <= '0';
			multA <= (others => '0');
			multB <= (others => '0');
			mem_writeMask <= (others => '1');
			out_mem_req <= '0';
			mem_addr <= (others => DontCareValue);
			mem_write <= (others => DontCareValue);
			inInterrupt <= '0';
		elsif (clk'event and clk = '1') then
			-- we must multiply unconditionally to get pipelined multiplication
        	tMultResult := multA * multB;
    		multResult3 <= multResult2;
    		multResult2 <= multResult;
    		multResult <= tMultResult(wordSize-1 downto 0);
    		
			
			spOffset(4):=not opcode(conv_integer(pc(byteBits-1 downto 0)))(4);
			spOffset(3 downto 0):=opcode(conv_integer(pc(byteBits-1 downto 0)))(3 downto 0);
			nextPC := pc + 1;

			-- prepare trace snapshot
			trace_opcode <= opcode(conv_integer(pc(byteBits-1 downto 0)));
			trace_pc <= pc;
			trace_sp <=	sp;
			trace_topOfStack <= stackA;
			trace_topOfStackB <= stackB;
			begin_inst <= '0';

			-- we terminate the requeset as soon as we get acknowledge
			if mem_ack = '1' then
				out_mem_req <= '0';
				mem_we <= '0';
			end if;
			
			if interrupt='0' then
				inInterrupt <= '0'; -- no longer in an interrupt
			end if;

			case state is
				when State_Idle =>
					if enable='1' then
						state <= State_Resync; 
					end if;
				-- Initial state of ZPU, fetch top of stack + first instruction 
				when State_Resync =>
					if mem_busy='0' then
						mem_addr <= sp;
						out_mem_req <= '1';
						state <= State_Resync2;
					end if;
				when State_Resync2 =>
					if mem_busy='0' then
						stackA <= mem_read;
						mem_addr <= incSp;
						out_mem_req <= '1';
						state <= State_Resync3;
					end if;
				when State_Resync3 =>
					if mem_busy='0' then
						stackB <= mem_read;
						mem_addr <= pc(maxAddrBitIncIO downto minAddrBit);
						out_mem_req <= '1';
						state <= State_Decode;
					end if;
				when State_Decode =>
					if mem_busy='0' then
						decodeWord <= mem_read;
						state <= State_Decode2;
					end if;
				when State_Decode2 =>
					-- decode 4 instructions in parallel
					for i in 0 to wordBytes-1 loop
						tOpcode := decodeWord((wordBytes-1-i+1)*8-1 downto (wordBytes-1-i)*8);

						tSpOffset(4):=not tOpcode(4);
						tSpOffset(3 downto 0):=tOpcode(3 downto 0);

						opcode(i) <= tOpcode;
						if (tOpcode(7 downto 7)=OpCode_Im) then
							tNextState:=State_Im;
						elsif (tOpcode(7 downto 5)=OpCode_StoreSP) then
							if tSpOffset = 0 then
								tNextState := State_Pop;
							elsif tSpOffset=1 then
								tNextState := State_PopDown;
							else
								tNextState :=State_StoreSP;
							end if;
						elsif (tOpcode(7 downto 5)=OpCode_LoadSP) then
							if tSpOffset = 0 then
								tNextState :=State_Dup;
							elsif tSpOffset = 1 then
								tNextState :=State_DupStackB;
							else
								tNextState :=State_LoadSP;
							end if;
						elsif (tOpcode(7 downto 5)=OpCode_Emulate) then
							tNextState :=State_Emulate;
							if tOpcode(5 downto 0)=OpCode_Neqbranch then
								tNextState :=State_Neqbranch;
							elsif tOpcode(5 downto 0)=OpCode_Eq then
								tNextState :=State_Eq;
							elsif tOpcode(5 downto 0)=OpCode_Lessthan then
								tNextState :=State_Lessthan;
							elsif tOpcode(5 downto 0)=OpCode_Lessthanorequal then
								--tNextState :=State_Lessthanorequal;
							elsif tOpcode(5 downto 0)=OpCode_Ulessthan then
								tNextState :=State_Ulessthan;
							elsif tOpcode(5 downto 0)=OpCode_Ulessthanorequal then
								--tNextState :=State_Ulessthanorequal;
							elsif tOpcode(5 downto 0)=OpCode_Loadb then
								tNextState :=State_Loadb;
							elsif tOpcode(5 downto 0)=OpCode_Mult then
								tNextState :=State_Mult;
							elsif tOpcode(5 downto 0)=OpCode_Storeb then
								tNextState :=State_Storeb;
							elsif tOpcode(5 downto 0)=OpCode_Pushspadd then
								tNextState :=State_Pushspadd;
							elsif tOpcode(5 downto 0)=OpCode_Callpcrel then
								tNextState :=State_Callpcrel;
							elsif tOpcode(5 downto 0)=OpCode_Call then
								--tNextState :=State_Call;
							elsif tOpcode(5 downto 0)=OpCode_Sub then
								tNextState :=State_Sub;
							elsif tOpcode(5 downto 0)=OpCode_PopPCRel then
								--tNextState :=State_PopPCRel;
							end if;								
						elsif (tOpcode(7 downto 4)=OpCode_AddSP) then
							if tSpOffset = 0 then
								tNextState := State_Shift;
							elsif tSpOffset = 1 then
								tNextState := State_AddTop;
							else
								tNextState :=State_AddSP;
							end if;
						else
							case tOpcode(3 downto 0) is
								when OpCode_Nop =>
									tNextState :=State_Nop;
								when OpCode_PushSP =>
									tNextState :=State_PushSP;
								when OpCode_PopPC =>
									tNextState :=State_PopPC;
								when OpCode_Add =>
									tNextState :=State_Add;
								when OpCode_Or =>
									tNextState :=State_Or;
								when OpCode_And =>
									tNextState :=State_And;
								when OpCode_Load =>
									tNextState :=State_Load;
								when OpCode_Not =>
									tNextState :=State_Not;
								when OpCode_Flip =>
									tNextState :=State_Flip;
								when OpCode_Store =>
									tNextState :=State_Store;
								when OpCode_PopSP =>
									tNextState :=State_PopSP;
								when others =>
									tNextState := State_Break;

							end case;
						end if;
						tDecodedOpcode(i) := tNextState;
						
					end loop;
					
					insn <= tDecodedOpcode(conv_integer(pc(byteBits-1 downto 0)));
					
					-- once we wrap, we need to fetch
					tDecodedOpcode(0) := State_InsnFetch;

					decodedOpcode <= tDecodedOpcode;
					state <= State_Execute;
						


					-- Each instruction must:
					--
					-- 1. set idim_flag
					-- 2. increase pc if applicable
					-- 3. set next state if appliable
					-- 4. do it's operation
					 
				when State_Execute =>
					insn <= decodedOpcode(conv_integer(nextPC(byteBits-1 downto 0)));
									
					case insn is
						when State_InsnFetch =>
							state <= State_Fetch;
						when State_Im =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '1';
								pc <= pc + 1;
								
								if idim_flag='1' then 
									stackA(wordSize-1 downto 7) <= stackA(wordSize-8 downto 0);
									stackA(6 downto 0) <= opcode(conv_integer(pc(byteBits-1 downto 0)))(6 downto 0);
								else
									out_mem_req <= '1';
									mem_we <= '1';
									mem_addr <= incSp;
									mem_write <= stackB;
									stackB <= stackA;
									sp <= decSp;
									for i in wordSize-1 downto 7 loop
										stackA(i) <= opcode(conv_integer(pc(byteBits-1 downto 0)))(6);
									end loop;
									stackA(6 downto 0) <= opcode(conv_integer(pc(byteBits-1 downto 0)))(6 downto 0);
								end if;
							else	
								insn <= insn;
							end if;
						when State_StoreSP =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								state <= State_StoreSP2;
								
								out_mem_req <= '1';
								mem_we <= '1';
								mem_addr <= sp+spOffset;
								mem_write <= stackA;
								stackA <= stackB;
								sp <= incSp;
							else	
								insn <= insn;
							end if;

					
						when State_LoadSP =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								state <= State_LoadSP2;
		
								sp <= decSp;
								out_mem_req <= '1';
								mem_we <= '1';
								mem_addr <= incSp;
								mem_write <= stackB;
							else	
								insn <= insn;
							end if;
						when State_Emulate =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								sp <= decSp;
								out_mem_req <= '1';
								mem_we <= '1';
								mem_addr <= incSp;
								mem_write <= stackB;
								stackA <= (others => DontCareValue);
								stackA(maxAddrBitIncIO downto 0) <= pc + 1;
								stackB <= stackA;
								
								-- The emulate address is:
								--        98 7654 3210
								-- 0000 00aa aaa0 0000
								pc <= (others => '0');
								pc(9 downto 5) <= opcode(conv_integer(pc(byteBits-1 downto 0)))(4 downto 0);
								state <= State_Fetch;
							else	
								insn <= insn;
							end if;
						when State_Callpcrel =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								stackA <= (others => DontCareValue);
								stackA(maxAddrBitIncIO downto 0) <= pc + 1;
								
								pc <= pc + stackA(maxAddrBitIncIO downto 0);
								state <= State_Fetch;
							else	
								insn <= insn;
							end if;
						when State_Call =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								stackA <= (others => DontCareValue);
								stackA(maxAddrBitIncIO downto 0) <= pc + 1;
								pc <= stackA(maxAddrBitIncIO downto 0);
								state <= State_Fetch;
							else	
								insn <= insn;
							end if;
						when State_AddSP =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								state <= State_AddSP2;
								
								out_mem_req <= '1';
								mem_addr <= sp+spOffset;
							else	
								insn <= insn;
							end if;
						when State_PushSP =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								pc <= pc + 1;
								
								sp <= decSp;
								stackA <= (others => '0');
								stackA(maxAddrBitIncIO downto minAddrBit) <= sp;
								stackB <= stackA;
								out_mem_req <= '1';
								mem_we <= '1';
								mem_addr <= incSp;
								mem_write <= stackB;
							else	
								insn <= insn;
							end if;
						when State_PopPC =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								pc <= stackA(maxAddrBitIncIO downto 0);
								sp <= incSp;
								
								out_mem_req <= '1';
								mem_we <= '1';
								mem_addr <= incSp;
								mem_write <= stackB;
								state <= State_Resync;
							else	
								insn <= insn;
							end if;
						when State_PopPCRel =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								pc <= stackA(maxAddrBitIncIO downto 0) + pc;
								sp <= incSp;
								
								out_mem_req <= '1';
								mem_we <= '1';
								mem_addr <= incSp;
								mem_write <= stackB;
								state <= State_Resync;
							else	
								insn <= insn;
							end if;
						when State_Add =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								stackA <= stackA + stackB;
								
								out_mem_req <= '1';
								mem_addr <= incIncSp;
								sp <= incSp;
								state <= State_Popped;
							else	
								insn <= insn;
							end if;
						when State_Sub =>
							begin_inst <= '1';
							idim_flag <= '0';
							binaryOpResult <= stackB - stackA;
							state <= State_BinaryOpResult;
						when State_Pop =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								mem_addr <= incIncSp;
								out_mem_req <= '1';
								sp <= incSp;
								stackA <= stackB;
								state <= State_Popped;						
							else	
								insn <= insn;
							end if;
						when State_PopDown =>
							if mem_busy='0' then
								-- PopDown leaves top of stack unchanged
								begin_inst <= '1';
								idim_flag <= '0';
								mem_addr <= incIncSp;
								out_mem_req <= '1';
								sp <= incSp;
								state <= State_Popped;						
							else	
								insn <= insn;
							end if;
						when State_Or =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								stackA <= stackA or stackB;
								out_mem_req <= '1';
								mem_addr <= incIncSp;
								sp <= incSp;
								state <= State_Popped;
							else	
								insn <= insn;
							end if;
						when State_And =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
		
								stackA <= stackA and stackB;
								out_mem_req <= '1';
								mem_addr <= incIncSp;
								sp <= incSp;
								state <= State_Popped;
							else	
								insn <= insn;
							end if;
						when State_Eq =>
							begin_inst <= '1';
							idim_flag <= '0';
	
		            		binaryOpResult <= (others => '0');
		                	if (stackA=stackB) then
		                		binaryOpResult(0) <= '1';
		                	end if;
							state <= State_BinaryOpResult;
		                when State_Ulessthan =>
							begin_inst <= '1';
							idim_flag <= '0';
	
		            		binaryOpResult <= (others => '0');
		                	if (stackA<stackB) then
		                		binaryOpResult(0) <= '1';
		                	end if;
							state <= State_BinaryOpResult;
		                when State_Ulessthanorequal =>
							begin_inst <= '1';
							idim_flag <= '0';
	
		            		binaryOpResult <= (others => '0');
		                	if (stackA<=stackB) then
		                		binaryOpResult(0) <= '1';
		                	end if;
							state <= State_BinaryOpResult;
		                when State_Lessthan =>
							begin_inst <= '1';
							idim_flag <= '0';
	
		            		binaryOpResult <= (others => '0');
		                	if (signed(stackA)<signed(stackB)) then
		                		binaryOpResult(0) <= '1';
		                	end if;
							state <= State_BinaryOpResult;
		                when State_Lessthanorequal =>
							begin_inst <= '1';
							idim_flag <= '0';
	
		            		binaryOpResult <= (others => '0');
		                	if (signed(stackA)<=signed(stackB)) then
		                		binaryOpResult(0) <= '1';
		                	end if;
							state <= State_BinaryOpResult;
						when State_Load =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								state <= State_Load2;
								
								mem_addr <= stackA(maxAddrBitIncIO downto minAddrBit);
								out_mem_req <= '1';
							else	
								insn <= insn;
							end if;

						when State_Dup =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								pc <= pc + 1; 
								
								sp <= decSp;
								stackB <= stackA;
								mem_write <= stackB;
								mem_addr <= incSp;
								out_mem_req <= '1';
								mem_we <= '1';
							else	
								insn <= insn;
							end if;
						when State_DupStackB =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								pc <= pc + 1; 
								
								sp <= decSp;
								stackA <= stackB;
								stackB <= stackA;
								mem_write <= stackB;
								mem_addr <= incSp;
								out_mem_req <= '1';
								mem_we <= '1';
							else	
								insn <= insn;
							end if;
						when State_Store =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								pc <= pc + 1;
								mem_addr <= stackA(maxAddrBitIncIO downto minAddrBit);
								mem_write <= stackB;
								out_mem_req <= '1';
								mem_we <= '1';
								sp <= incIncSp;
								state <= State_Resync;
							else	
								insn <= insn;
							end if;
						when State_PopSP =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								pc <= pc + 1;
								
								mem_write <= stackB;
								mem_addr <= incSp;
								out_mem_req <= '1';
								mem_we <= '1';
								sp <= stackA(maxAddrBitIncIO downto minAddrBit);
								state <= State_Resync;
							else	
								insn <= insn;
							end if;
						when State_Nop =>	
							begin_inst <= '1';
							idim_flag <= '0';
							pc <= pc + 1;
						when State_Not =>
							begin_inst <= '1';
							idim_flag <= '0';
							pc <= pc + 1; 
							
							stackA <= not stackA;
						when State_Flip =>
							begin_inst <= '1';
							idim_flag <= '0';
							pc <= pc + 1; 
							
							for i in 0 to wordSize-1 loop
								stackA(i) <= stackA(wordSize-1-i);
				  			end loop;
						when State_AddTop =>
							begin_inst <= '1';
							idim_flag <= '0';
							pc <= pc + 1; 
							
							stackA <= stackA + stackB;
						when State_Shift =>
							begin_inst <= '1';
							idim_flag <= '0';
							pc <= pc + 1; 
							
							stackA(wordSize-1 downto 1) <= stackA(wordSize-2 downto 0);
							stackA(0) <= '0';
						when State_Pushspadd =>
							begin_inst <= '1';
							idim_flag <= '0';
							pc <= pc + 1; 
							
							stackA <= (others => '0');
							stackA(maxAddrBitIncIO downto minAddrBit) <= stackA(maxAddrBitIncIO-minAddrBit downto 0)+sp;
						when State_Neqbranch =>
							-- branches are almost always taken as they form loops
							begin_inst <= '1';
							idim_flag <= '0';
							sp <= incIncSp;
		                	if (stackB/=0) then
		                		pc <= stackA(maxAddrBitIncIO downto 0) + pc;
		                	else
		                		pc <= pc + 1;
		                	end if;		
		                	-- need to fetch stack again.				
							state <= State_Resync;
		                when State_Mult =>
							begin_inst <= '1';
							idim_flag <= '0';
		
							multA <= stackA;
							multB <= stackB;
							state <= State_Mult2;
						when State_Break =>
							report "Break instruction encountered" severity failure;
							break <= '1';

						when State_Loadb =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								state <= State_Loadb2;
								
								mem_addr <= stackA(maxAddrBitIncIO downto minAddrBit);
								out_mem_req <= '1';
							else	
								insn <= insn;
							end if;
						when State_Storeb =>
							if mem_busy='0' then
								begin_inst <= '1';
								idim_flag <= '0';
								state <= State_Storeb2;
								
								mem_addr <= stackA(maxAddrBitIncIO downto minAddrBit);
								out_mem_req <= '1';
							else	
								insn <= insn;
							end if;
				
						when others =>
--							sp <= (others => DontCareValue);
							report "Illegal instruction" severity failure;
							break <= '1';
					end case;


				when State_StoreSP2 =>
					if mem_busy='0' then
						mem_addr <= incSp;
						out_mem_req <= '1';
						state <= State_Popped;
					end if;
				when State_LoadSP2 =>
					if mem_busy='0' then
						state <= State_LoadSP3;
						out_mem_req <= '1';
						mem_addr <= sp+spOffset+1;
					end if;
				when State_LoadSP3 =>
					if mem_busy='0' then
						pc <= pc + 1;
						state <= State_Execute;
						stackB <= stackA;
						stackA <= mem_read;
					end if;
				when State_AddSP2 =>
					if mem_busy='0' then
						pc <= pc + 1;
						state <= State_Execute;
						stackA <= stackA + mem_read;
					end if;
				when State_Load2 =>
					if mem_busy='0' then
						stackA <= mem_read;
						pc <= pc + 1;
						state <= State_Execute;
					end if;
				when State_Loadb2 =>
					if mem_busy='0' then
						stackA <= (others => '0');
						stackA(7 downto 0) <= mem_read(((wordBytes-1-conv_integer(stackA(byteBits-1 downto 0)))*8+7) downto (wordBytes-1-conv_integer(stackA(byteBits-1 downto 0)))*8);
						pc <= pc + 1;
						state <= State_Execute;
					end if;
				when State_Storeb2 =>
					if mem_busy='0' then
						mem_addr <= stackA(maxAddrBitIncIO downto minAddrBit);
						mem_write <= mem_read;
						mem_write(((wordBytes-1-conv_integer(stackA(byteBits-1 downto 0)))*8+7) downto (wordBytes-1-conv_integer(stackA(byteBits-1 downto 0)))*8) <= stackB(7 downto 0) ;
						out_mem_req <= '1';
						mem_we <= '1';
						pc <= pc + 1;
						sp <= incIncSp;
						state <= State_Resync;
					end if;
				when State_Fetch =>
					if mem_busy='0' then
						if interrupt='1' and inInterrupt='0' and idim_flag='0' then
							-- We got an interrupt
							inInterrupt <= '1';
							
							sp <= decSp;
							out_mem_req <= '1';
							mem_we <= '1';
							mem_addr <= incSp;
							mem_write <= stackB;
							stackA <= (others => DontCareValue);
							stackA(maxAddrBitIncIO downto 0) <= pc;
							stackB <= stackA;
							
							pc <= conv_std_logic_vector(32, maxAddrBitIncIo+1); -- interrupt address
							
			  				report "ZPU jumped to interrupt!" severity note;
						else
							mem_addr <= pc(maxAddrBitIncIO downto minAddrBit);
							out_mem_req <= '1';
							state <= State_Decode;
						end if;
					end if;
                when State_Mult2 =>
					state <= State_Mult3;
                when State_Mult3 =>
					state <= State_Mult4;
                when State_Mult4 =>
					state <= State_Mult5;
                when State_Mult5 =>
            		stackA <= multResult3;
                	state <= State_Mult6;
                when State_Mult6 =>
					if mem_busy='0' then
						out_mem_req <= '1';
						mem_addr <= incIncSp;
						sp <= incSp;
						state <= State_Popped;
					end if;
				when State_BinaryOpResult =>
					if mem_busy='0' then
						-- NB!!!! we know that the memory isn't busy at this point!!!!
						out_mem_req <= '1';
						mem_addr <= incIncSp;
						sp <= incSp;
						stackA <= binaryOpResult;
						state <= State_Popped;
					end if;
				when State_Popped =>
					if mem_busy='0' then
						pc <= pc + 1;
						stackB <= mem_read;
						state <= State_Execute;
					end if;
				when others =>	
--					sp <= (others => DontCareValue);
					report "Illegal state" severity failure;
					break <= '1';
			end case;
		end if;
	end process;



end behave;
