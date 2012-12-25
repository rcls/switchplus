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

library unisim;
use unisim.vcomponents.all;

entity tmds_channel is
  port (D : in byte_t;
        C0, C1 : in std_logic;
        DE : in std_logic;
        Qp, Qn : out std_logic;
        clk, clk_nibble, ioclk, lock, serdesstrobe : in std_logic);
end tmds_channel;
architecture tmds_channel of tmds_channel is
  signal buf, wordA, wordB, bufA, bufB : dec_t;
  signal which, whichB, whichC : std_logic := '0';
  signal nibble : nibble_t;
  signal Q : std_logic;
  signal phase : integer range 0 to 4 := 0;
begin
  encode : entity work.tmds_encode_byte port map (D, C0, C1, DE, buf, clk);
  process
  begin
    wait until rising_edge (clk);
    which <= not which;
    if which = '1' then
      bufA <= buf;
    else
      bufB <= buf;
    end if;
  end process;
  process
  begin
    wait until rising_edge (clk_nibble);
    -- If which just transitioned to 0, then we just wrote A.
    whichB <= which;
    whichC <= whichB;
    wordA <= bufA;
    wordB <= bufB;
    if (whichC = '1' and whichB ='0') or phase = 4 then
      phase <= 0;
    else
      phase <= phase + 1;
    end if;
    case phase is
      when 0 => nibble <= wordA(3 downto 0);
      when 1 => nibble <= wordA(7 downto 4);
      when 2 => nibble <= wordB(1 downto 0) & wordA(9 downto 8);
      when 3 => nibble <= wordB(5 downto 2);
      when 4 => nibble <= wordB(9 downto 6);
    end case;
  end process;

  osd : oserdes2
    generic map (DATA_WIDTH=> 4, DATA_RATE_OQ=> "SDR", DATA_RATE_OT=> "SDR")
    port map (D1=>nibble(0), D2=> nibble(1), D3=> nibble(2), D4=>nibble(3),
              CLK0=> ioclk, CLKDIV=> clk_nibble, IOCE=> serdesstrobe, OQ=> Q,
              SHIFTIN1=>'0', SHIFTIN2=>'0', SHIFTIN3=>'0', SHIFTIN4=>'0',
              T1=>'0', T2=>'0', T3=>'0', T4=>'0',
              RST=> '0', TCE=> '1', TRAIN=>'0', CLK1=> '0');
  obd : obufds port map (I=> Q, O=> Qp, OB=> Qn);
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
        clk, clk_nibble, clk_bit, locked : in std_logic);
end tmds_encode;
architecture tmds_encode of tmds_encode is
  signal ioclk, lock, serdesstrobe : std_logic;
  signal clk180 : std_logic;
  signal Rnibble, Gnibble, Bnibble : nibble_t;
  signal Rq, Gq, Bq, Cq : std_logic;
  attribute keep_hierarchy of tmds_encode : architecture is "soft";
begin
  Rc : entity work.tmds_channel port map (
    R, ctl3, ctl2, DE, Rp, Rn, clk, clk_nibble, ioclk, lock, serdesstrobe);
  Gc : entity work.tmds_channel port map (
    G, ctl0, ctl1, DE, Gp, Gn, clk, clk_nibble, ioclk, lock, serdesstrobe);
  Bc : entity work.tmds_channel port map (
    B, hsync, vsync, DE, Bp, Bn, clk, clk_nibble, ioclk, lock, serdesstrobe);

  clk180 <= not clk;
  --clk180 <= clk;
  Co : oddr2
    port map (D0=>'1', D1=>'0', CE=>'1', Q=> Cq, C0=>clk, C1=>clk180);
  Cd : obufds port map (I=> Cq, O=> Cp, OB=> Cn);
  bpl : bufpll generic map (DIVIDE=>4)
    port map (GCLK=> clk_nibble, PLLIN=> clk_bit, LOCKED=> locked,
              IOCLK=> ioclk, SERDESSTROBE=>serdesstrobe, LOCK=> lock);
end tmds_encode;
