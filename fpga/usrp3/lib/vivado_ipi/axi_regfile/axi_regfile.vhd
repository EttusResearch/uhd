library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity axi_regfile is
generic (
	NUM_REGS : integer := 16
);
port (
	regs : out std_logic_vector(NUM_REGS*32-1 downto 0);

	S_AXI_ACLK	: in std_logic;
	S_AXI_ARESETN	: in std_logic;
	S_AXI_AWADDR	: in std_logic_vector(11 downto 0);
	S_AXI_AWVALID	: in std_logic;
	S_AXI_AWREADY	: out std_logic;
	S_AXI_WDATA	: in std_logic_vector(31 downto 0);
	S_AXI_WSTRB	: in std_logic_vector(3 downto 0);
	S_AXI_WVALID	: in std_logic;
	S_AXI_WREADY	: out std_logic;
	S_AXI_BRESP	: out std_logic_vector(1 downto 0);
	S_AXI_BVALID	: out std_logic;
	S_AXI_BREADY	: in std_logic;
	S_AXI_ARADDR	: in std_logic_vector(11 downto 0);
	S_AXI_ARVALID	: in std_logic;
	S_AXI_ARREADY	: out std_logic;
	S_AXI_RDATA	: out std_logic_vector(31 downto 0);
	S_AXI_RRESP	: out std_logic_vector(1 downto 0);
	S_AXI_RVALID	: out std_logic;
	S_AXI_RREADY	: in std_logic
);
end axi_regfile;

architecture arch of axi_regfile is
	type regfile_t is array (integer range <>) of std_logic_vector(31 downto 0);
	signal read_token : std_logic;
	signal write_addr : std_logic_vector(S_AXI_AWADDR'left downto S_AXI_AWADDR'right);
	signal write_strb : std_logic_vector(3 downto 0);
	signal write_addr_token : std_logic;
	signal write_data : std_logic_vector(31 downto 0);
	signal write_data_token : std_logic;
	signal soft_reset : std_logic;
	signal regs_r     : regfile_t(NUM_REGS-1 downto 0);
begin

	S_AXI_ARREADY <= not read_token;
	S_AXI_RVALID <= read_token;
	S_AXI_RRESP <= "00";

	S_AXI_AWREADY <= not write_addr_token;
	S_AXI_WREADY <= not write_data_token;
	S_AXI_BVALID <= write_addr_token and write_data_token;
	S_AXI_BRESP <= "00";

	--Port assignment from registers
	reg_distribution : process (regs_r)
	begin
		for i in 0 to NUM_REGS-1 loop
			regs(32*(i+1)-1 downto 32*i) <= regs_r(i);
		end loop;
	end process reg_distribution;

	--Register reads
	read_proc : process (S_AXI_ACLK)
		variable read_addr : integer; 
	begin
	if rising_edge(S_AXI_ACLK) then
		read_addr := to_integer(unsigned(S_AXI_ARADDR(S_AXI_ARADDR'left downto S_AXI_ARADDR'right+2)));

		if (S_AXI_ARESETN = '0') then
			read_token <= '0';
		elsif (S_AXI_ARVALID = '1') and (read_token = '0') then
			read_token <= '1';
		elsif (S_AXI_RREADY = '1') and (read_token = '1') then
			read_token <= '0';
		end if;

		if (S_AXI_ARVALID = '1') and (read_token = '0') then
			S_AXI_RDATA <= (others => '0');
			for i in 0 to NUM_REGS-1 loop
				if (read_addr = i) then
					S_AXI_RDATA <= regs_r(i);
				end if;
			end loop;
		end if;
	end if;
	end process read_proc;

	write_proc : process (S_AXI_ACLK)
	begin
	if rising_edge(S_AXI_ACLK) then
		if (S_AXI_ARESETN = '0') then
			write_addr_token <= '0';
			write_data_token <= '0';
			write_strb <= (others => '0');
		else
			if (S_AXI_AWVALID = '1') and (write_addr_token = '0') then
				write_addr_token <= '1';
			elsif (S_AXI_BREADY = '1') and (write_addr_token = '1') and (write_data_token = '1') then
				write_addr_token <= '0';
			end if;

			if (S_AXI_WVALID = '1') and (write_data_token = '0') then
				write_data_token <= '1';
			elsif (S_AXI_BREADY = '1') and (write_addr_token = '1') and (write_data_token = '1') then
				write_data_token <= '0';
			end if;
		end if;

		if (S_AXI_AWVALID = '1') and (write_addr_token = '0') then
			write_addr <= S_AXI_AWADDR;
		end if;

		if (S_AXI_WVALID = '1') and (write_data_token = '0') then
			write_data <= S_AXI_WDATA;
			write_strb <= S_AXI_WSTRB;
		end if;
	end if;
	end process write_proc;

	-- Update registers on write
	write_reg : process (S_AXI_ACLK)
		variable write_addr_int : integer;
	begin
	if rising_edge(S_AXI_ACLK) then
		write_addr_int := to_integer(unsigned(write_addr(write_addr'left downto 2)));

		if (S_AXI_ARESETN = '0') or (soft_reset = '1') then
			--Initial states for each signal
			soft_reset <= '0';
		elsif (write_addr_token = '1') and (write_data_token = '1') then
			for i in 0 to NUM_REGS-1 loop
				if (write_addr_int = i) then
					for j in write_strb'left downto write_strb'right loop
						regs_r(i)(j*8+7 downto j*8) <= write_data(j*8+7 downto j*8);
					end loop;
				end if;
			end loop;
		end if;
	end if;
	end process write_reg;

end arch;

