library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.defs.all;

entity tmds_encode_byte is
  port (D : in byte_t;
        C0, C1 : in std_logic;
        DE : in std_logic;
        Q : out dec_t;
        clk : in std_logic);
end tmds_encode_byte;
architecture tmds_encode_byte of tmds_encode_byte is
  signal encode_rom : encode_rom_t := encode_rom_f(0);
  signal encodings : u28;
  signal count : unsigned(3 downto 0) := x"0";
  signal C0e, C1e, DEe : std_logic;
begin
  process
    variable addend : unsigned(3 downto 0);
  begin
    wait until rising_edge(clk);
    encodings <= encode_rom(to_integer(D));
    C0e <= C0;
    C1e <= C1;
    DEe <= DE;
    if DEe = '1' then
      if count(3) = '1' then
        count <= count + encodings(27 downto 24);
        Q <= encodings(23 downto 14);
      else
        count <= count + encodings(13 downto 10);
        Q <= encodings(9 downto 0);
      end if;
    else
      if C1e = '0' then
        if C0e = '0' then
          Q <= "0010101011";
        else
          Q <= "1101010100";
        end if;
      else
        if C0e = '0' then
          Q <= "0010101010";
        else
          Q <= "1101010101";
        end if;
      end if;
    end if;
  end process;
end tmds_encode_byte;

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.defs.all;

entity tmds_channel is
  port (D : in byte_t;
        C0, C1 : in std_logic;
        DE : in std_logic;
        Q : out unsigned(3 downto 0);
        clk : in std_logic;
        clk_nibble : in std_logic);
end tmds_channel;
architecture tmds_channel of tmds_channel is
  signal buf, wordA, wordB : dec_t;
  signal which, whichB : std_logic := '0';
  signal phase : integer range 0 to 4 := 0;
begin
  encode : entity work.tmds_encode_byte port map (D, C0, C1, DE, buf, clk);
  process
  begin
    wait until rising_edge (clk);
    which <= not which;
    if which = '1' then
      wordA <= buf;
    else
      wordB <= buf;
    end if;
  end process;
  process
  begin
    wait until rising_edge (clk_nibble);
    -- If which just transitioned to 0, then we just wrote A.
    whichB <= which;
    if (whichB = '1' and which = '0') or phase = 4 then
      phase <= 0;
    else
      phase <= phase + 1;
    end if;
    case phase is
      when 0 => Q <= wordA(3 downto 0);
      when 1 => Q <= wordA(7 downto 4);
      when 2 => Q <= wordB(1 downto 0) & wordA(9 downto 8);
      when 3 => Q <= wordB(5 downto 2);
      when 4 => Q <= wordB(9 downto 6);
    end case;
  end process;
end tmds_channel;

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.defs.all;

library unisim;
use unisim.vcomponents.all;

entity tmds_encode is
  port (R, G, B : in byte_t;
        hsync, vsync : in std_logic;
        ctl0, ctl1, ctl2, ctl3 : in std_logic;
        DE : in std_logic;
        Rp, Rn, Gp, Gn, Bp, Bn, Cp, Cn : out std_logic;
        clk, clk_nibble, clk_bit, clk_bit180, locked : in std_logic);
end tmds_encode;
architecture tmds_encode of tmds_encode is
  signal ioclk, ioclk180, lock, serdesstrobe : std_logic;
  signal clk180 : std_logic;
  signal Rnibble, Gnibble, Bnibble : nibble_t;
  signal Rq, Rt, Gq, Gt, Bq, Bt, Cq : std_logic;
  attribute keep_hierarchy of tmds_encode : architecture is "soft";
begin
  Rc : entity work.tmds_channel port map (
    R, ctl2, ctl3, DE, Rnibble, clk, clk_nibble);
  Gc : entity work.tmds_channel port map (
    G, ctl1, ctl0, DE, Gnibble, clk, clk_nibble);
  Bc : entity work.tmds_channel port map (
    B, hsync, vsync, DE, Bnibble, clk, clk_nibble);

  Ro : oserdes2 generic map (DATA_WIDTH=> 4, DATA_RATE_OQ=> "DDR")
    port map (D1=>Rnibble(0), D2=> Rnibble(1), D3=> Rnibble(2), D4=>Rnibble(3),
              T1=>'0', T2=>'0', T3=>'0', T4=>'0',
              SHIFTIN1=>'0', SHIFTIN2=>'0', SHIFTIN3=>'0', SHIFTIN4=>'0',
              CLK0=> ioclk, CLK1=> ioclk180, CLKDIV=> clk_nibble,
              IOCE=> serdesstrobe, RST=> '0',
              TCE=> '1', TRAIN=>'0',
              -- OCE=>'0', TCE=>'0',
              OQ=> Rq, TQ=> Rt);
  Go : oserdes2 generic map (DATA_WIDTH=> 4, DATA_RATE_OQ=> "DDR")
    port map (D1=>Gnibble(0), D2=> Gnibble(1), D3=> Gnibble(2), D4=>Gnibble(3),
              T1=>'0', T2=>'0', T3=>'0', T4=>'0',
              SHIFTIN1=>'0', SHIFTIN2=>'0', SHIFTIN3=>'0', SHIFTIN4=>'0',
              CLK0=> ioclk, CLK1=> ioclk180, CLKDIV=> clk_nibble,
              IOCE=> serdesstrobe, RST=> '0',
              TCE=> '1', TRAIN=>'0',
              -- OCE=>1, TCE=>1,
              OQ=> Gq, TQ=> Gt);
  Bo : oserdes2 generic map (DATA_WIDTH=> 4, DATA_RATE_OQ=> "DDR")
    port map (D1=>Bnibble(0), D2=> Bnibble(1), D3=> Bnibble(2), D4=>Bnibble(3),
              T1=>'0', T2=>'0', T3=>'0', T4=>'0',
              SHIFTIN1=>'0', SHIFTIN2=>'0', SHIFTIN3=>'0', SHIFTIN4=>'0',
              CLK0=> ioclk, CLK1=> ioclk180, CLKDIV=> clk_nibble,
              IOCE=> serdesstrobe, RST=> '0',
              TCE=> '1', TRAIN=>'0',
              -- OCE=>1, TCE=>1,
              OQ=> Bq, TQ=> Bt);
  clk180 <= not clk;
  --clk180 <= clk;
  Co : oddr2
    port map (D0=>'1', D1=>'0', CE=>'1', Q=> Cq, C0=>clk, C1=>clk180);
  Rd : obuftds port map (I=> Rq, T=> Rt, O=> Rp, OB=> Rn);
  Gd : obuftds port map (I=> Gq, T=> Gt, O=> Gp, OB=> Gn);
  Bd : obuftds port map (I=> Bq, T=> Bt, O=> Bp, OB=> Bn);
  Cd : obufds port map (I=>Cq, O=> Cp, OB=>Cn);
  bpl : bufpll generic map (DIVIDE=>2)
    port map (GCLK=> clk_nibble, PLLIN=> clk_bit, LOCKED=> locked,
              IOCLK=> ioclk, SERDESSTROBE=>serdesstrobe, LOCK=> lock);
  bpl180 : bufpll generic map (DIVIDE=>2)
    port map (GCLK=> clk_nibble, PLLIN=> clk_bit180, LOCKED=> locked,
              IOCLK=> ioclk180);
end tmds_encode;
