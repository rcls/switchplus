library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.defs.all;

entity console is
  port (in_R, in_G, in_B : in byte_t;
        in_hsync, in_vsync, in_de : in std_logic;

        out_R, out_G, out_B : out byte_t;
        out_hsync, out_vsync, out_de : out std_logic;
        lcd_clk : in std_logic;

        spi_sel, spi_in, spi_clk : in std_logic;

        spi_nearly_full : out std_logic;

        clk : in std_logic);
end console;

architecture console of console is
  signal spi_bit_count : unsigned (3 downto 0) := x"0";
  signal spi_byte : byte_t;

  signal in_flag, in_flag_prev : std_logic;

  subtype fifo_address_t is unsigned (10 downto 0);
  type byte_2048_t is array (0 to 2047) of byte_t;

  signal in_count, fifo_count : fifo_address_t;
  signal fifo : byte_2048_t;

  signal fifo_byte : byte_t;
  signal fifo_byte_valid : boolean;

  subtype text_address_t is unsigned (12 downto 0);
  type byte_8192_t is array (0 to 8191) of byte_t;
  signal text_buffer : byte_8192_t := (others => x"a0");

  --signal text_start : text_address_t := "0000000000000";
  constant text_start_x : unsigned (6 downto 0) := "0000000";
  signal   text_start_y : unsigned (5 downto 0) := "000000";

  signal cursor_address : text_address_t := "0000000000000";
  signal clear_address : text_address_t;

  signal write_address : text_address_t;
  signal write_byte : byte_t;
  signal write_valid : boolean;

  constant last_line : unsigned (5 downto 0) := "111111";

  signal pixel_count : unsigned (19 downto 0);
  signal read_byte : byte_t;
  signal opaque : std_logic;
  signal text_bit : std_logic;

  alias cursor_x is cursor_address (6 downto 0);
  alias cursor_y is cursor_address (12 downto 7);
  --alias text_start_x is text_start (6 downto 0);
  --alias text_start_y is text_start (12 downto 7);
  alias clear_address_x is clear_address(6 downto 0);
  alias clear_address_y is clear_address(12 downto 7);

  alias pixel_char_x is pixel_count (2 downto 0);
  alias pixel_text_x is pixel_count (9 downto 3);
  alias pixel_char_y is pixel_count (13 downto 10);
  alias pixel_text_y is pixel_count (19 downto 14);

  signal pixel_char_x2 : unsigned (2 downto 0);
  signal pixel_char_y2 : unsigned (3 downto 0);

  signal R2, G2, B2, R3, G3, B3 : byte_t;
  signal de2, de3, hsync2, hsync3, vsync2, vsync3 : std_logic;

  type console_state_t is (cs_normal, cs_escape, cs_command, cs_clearing);
  signal console_state : console_state_t;

  constant char_gen : std_logic_vector (0 to 16383) := (
    x"00000000000000000000000000000000" & x"00000000000000000000000000000000" &
    x"00000000000000000000000000000000" & x"00000000000000000000000000000000" &
    x"00000000000000000000000000000000" & x"00000000000000000000000000000000" &
    x"00000000000000000000000000000000" & x"00000000000000000000000000000000" &
    x"00000000000000000000000000000000" & x"00000000000000000000000000000000" &
    x"00000000000000000000000000000000" & x"00000000000000000000000000000000" &
    x"00000000000000000000000000000000" & x"00000000000000000000000000000000" &
    x"00000000000000000000000000000000" & x"00000000000000000000000000000000" &
    x"00000000000000000000000000000000" & x"00000000000000000000000000000000" &
    x"00000000000000000000000000000000" & x"00000000000000000000000000000000" &
    x"00000000000000000000000000000000" & x"00000000000000000000000000000000" &
    x"00000000000000000000000000000000" & x"00000000000000000000000000000000" &
    x"00000000000000000000000000000000" & x"00000000000000000000000000000000" &
    x"00000000000000000000000000000000" & x"00000000000000000000000000000000" &
    x"00000000000000000000000000000000" & x"00000000000000000000000000000000" &
    x"00000000000000000000000000000000" & x"00000000000000000000000000000000" &
    x"00000000000000000000000000000000" & x"00000000080808080808080008080000" &
    x"00002222222200000000000000000000" & x"000000001212127e24247e4848480000" &
    x"00000000083e4948380e09493e080000" & x"00000000314a4a340808162929460000" &
    x"000000001c2222221c39454246390000" & x"00000808080800000000000000000000" &
    x"00000004080810101010101008080400" & x"00000020101008080808080810102000" &
    x"00000000000008492a1c2a4908000000" & x"0000000000000808087f080808000000" &
    x"00000000000000000000000018080810" & x"0000000000000000007e000000000000" &
    x"00000000000000000000000018180000" & x"00000000020204080810102040400000" &
    x"00000000182442424242424224180000" & x"000000000818280808080808083e0000" &
    x"000000003c4242020c102040407e0000" & x"000000003c4242021c020242423c0000" &
    x"00000000040c142444447e0404040000" & x"000000007e4040407c020202423c0000" &
    x"000000001c2040407c424242423c0000" & x"000000007e0202040404080808080000" &
    x"000000003c4242423c424242423c0000" & x"000000003c4242423e02020204380000" &
    x"00000000000018180000001818000000" & x"00000000000018180000001808081000" &
    x"00000000000204081020100804020000" & x"000000000000007e0000007e00000000" &
    x"00000000004020100804081020400000" & x"000000003c4242020408080008080000" &
    x"000000001c224a565252524e201e0000" & x"0000000018242442427e424242420000" &
    x"000000007c4242427c424242427c0000" & x"000000003c42424040404042423c0000" &
    x"00000000784442424242424244780000" & x"000000007e4040407c404040407e0000" &
    x"000000007e4040407c40404040400000" & x"000000003c424240404e4242463a0000" &
    x"00000000424242427e42424242420000" & x"000000003e08080808080808083e0000" &
    x"000000001f0404040404044444380000" & x"00000000424448506060504844420000" &
    x"000000004040404040404040407e0000" & x"00000000424266665a5a424242420000" &
    x"0000000042626252524a4a4646420000" & x"000000003c42424242424242423c0000" &
    x"000000007c4242427c40404040400000" & x"000000003c4242424242425a663c0300" &
    x"000000007c4242427c48444442420000" & x"000000003c424240300c0242423c0000" &
    x"000000007f0808080808080808080000" & x"000000004242424242424242423c0000" &
    x"00000000414141222222141408080000" & x"00000000424242425a5a666642420000" &
    x"00000000424224241818242442420000" & x"00000000414122221408080808080000" &
    x"000000007e02020408102040407e0000" & x"0000000e080808080808080808080e00" &
    x"00000000404020101008080402020000" & x"00000070101010101010101010107000" &
    x"00001824420000000000000000000000" & x"00000000000000000000000000007f00" &
    x"00201008000000000000000000000000" & x"0000000000003c42023e4242463a0000" &
    x"0000004040405c6242424242625c0000" & x"0000000000003c4240404040423c0000" &
    x"0000000202023a4642424242463a0000" & x"0000000000003c42427e4040423c0000" &
    x"0000000c1010107c1010101010100000" & x"0000000000023a44444438203c42423c" &
    x"0000004040405c624242424242420000" & x"000000080800180808080808083e0000" &
    x"0000000404000c040404040404044830" & x"00000000404044485060504844420000" &
    x"000000001808080808080808083e0000" & x"00000000000076494949494949490000" &
    x"0000000000005c624242424242420000" & x"0000000000003c4242424242423c0000" &
    x"0000000000005c6242424242625c4040" & x"0000000000003a4642424242463a0202" &
    x"0000000000005c624240404040400000" & x"0000000000003c4240300c02423c0000" &
    x"0000000010107c1010101010100c0000" & x"000000000000424242424242463a0000" &
    x"00000000000042424224242418180000" & x"00000000000041494949494949360000" &
    x"00000000000042422418182442420000" & x"0000000000004242424242261a02023c" &
    x"0000000000007e0204081020407e0000" & x"0000000c101008081010080810100c00" &
    x"00000808080808080808080808080808" & x"00000030080810100808101008083000" &
    x"00000031494600000000000000000000" & x"00000000000000000000000000000000"
    );

begin
  -- The spi input is driven of spi_clk; spi_sel is implemented as an async
  -- reset.  So long as the main clk is at least an eighth of the spi_clk speed,
  -- it will see every byte.
  process (spi_clk, spi_sel)
  begin
    if spi_sel = '1' then
      spi_bit_count(2 downto 0) <= "000";
    elsif rising_edge(spi_clk) then
      spi_byte <= spi_byte(6 downto 0) & spi_in;
      spi_bit_count <= spi_bit_count + 1;
    end if;
  end process;

  process -- fifo loading.
    variable used : unsigned (10 downto 0);
  begin
    wait until rising_edge(clk);
    in_flag <= spi_bit_count(3);
    in_flag_prev <= in_flag;

    if in_flag /= in_flag_prev then
      fifo(to_integer(in_count)) <= spi_byte;
      in_count <= in_count + 1;
    end if;

    used := in_count - fifo_count;
    spi_nearly_full <= used(10);
  end process;

  process -- console to text buffer.
    variable next_console_state : console_state_t;
    variable cursor_on_last_line : boolean;
    variable scroll : std_logic;
  begin
    wait until rising_edge(clk);

    next_console_state := console_state;
    cursor_on_last_line := (cursor_y - text_start_y = last_line);
    clear_address <= (others=>'X');
    write_address <= (others=>'X');
    write_byte <= (others=>'X');
    write_valid <= false;

    scroll := '0';
    if console_state = cs_normal and fifo_byte_valid then
      if fifo_byte(7 downto 5) /= "000" then
        write_address <= cursor_address;
        write_byte <= fifo_byte;
        write_valid <= true;
        cursor_address <= cursor_address + 1;
        if cursor_x = "1111111" and cursor_on_last_line then
          scroll := '1';
        end if;
      end if;
      if fifo_byte = x"0a" then
        cursor_x <= "0000000";
        cursor_y <= cursor_y + 1;
        if cursor_on_last_line then
          scroll := '1';
        end if;
      end if;
      if fifo_byte = x"0d" then
        cursor_x <= "0000000";
      end if;
      if fifo_byte = x"1b" then
        next_console_state := cs_escape;
      end if;
    end if;

    if console_state = cs_escape and fifo_byte_valid then
      if fifo_byte = x"5b" then
        next_console_state := cs_command;
      else
        next_console_state := cs_normal;
      end if;
    end if;

    if console_state = cs_command and fifo_byte_valid then
      if fifo_byte = x"4b" then
        next_console_state := cs_clearing;
        clear_address <= cursor_address;
      else
        next_console_state := cs_normal;
      end if;
    end if;

    if console_state = cs_clearing then
      clear_address <= clear_address + 1;
      write_address <= clear_address;
      write_byte <= x"a0";
      write_valid <= true;
      if clear_address_x = "1111111" then
        next_console_state := cs_normal;
      end if;
    end if;

    text_start_y <= text_start_y + ("00000" & scroll);
    if scroll = '1' then
      next_console_state := cs_clearing;
      clear_address <= text_start_y & text_start_x;
    end if;

    if write_valid then
      text_buffer(to_integer(write_address)) <= write_byte;
    end if;

    console_state <= next_console_state;
    fifo_byte_valid <= false;
    fifo_byte <= "XXXXXXXX";
    if next_console_state /= cs_clearing and in_count /= fifo_count then
      fifo_byte_valid <= true;
      fifo_count <= fifo_count + 1;
      fifo_byte <= fifo(to_integer(fifo_count));
    end if;
  end process;

  process -- text render.
  begin
    wait until rising_edge(lcd_clk);

    if in_de = '0' and in_vsync /= vsync2 then
      pixel_text_x <= text_start_x;
      pixel_text_y <= text_start_y;
      pixel_char_x <= "000";
      pixel_char_y <= "0000";
    end if;
    if in_de = '1' then
      pixel_count <= pixel_count + 1;
    end if;

    read_byte <= text_buffer(to_integer(pixel_text_y & pixel_text_x));
    de2 <= in_de;
    vsync2 <= in_vsync;
    hsync2 <= in_hsync;
    R2 <= in_R;
    G2 <= in_G;
    B2 <= in_B;
    pixel_char_y2 <= pixel_char_y;
    pixel_char_x2 <= pixel_char_x;

    opaque <= not read_byte(7);
    text_bit <= char_gen(to_integer(
      read_byte(6 downto 0) & pixel_char_y2 & pixel_char_x2));
    de3 <= de2;
    vsync3 <= vsync2;
    hsync3 <= hsync2;
    R3 <= R2;
    G3 <= G2;
    B3 <= B2;

    out_de <= de3;
    out_vsync <= vsync3;
    out_hsync <= hsync3;
    if text_bit = '1' then
      out_R <= x"ff";
      out_G <= x"ff";
      out_B <= x"ff";
    elsif opaque = '1' then
      out_R <= x"00";
      out_G <= x"00";
      out_B <= x"00";
    else
      out_R <= R3;
      out_G <= G3;
      out_B <= B3;
    end if;
  end process;
end console;
