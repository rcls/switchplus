library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity watchdog is
  generic (bits : integer);
  port (I : in std_logic;
        Ok : out std_logic;
        clk : in std_logic);
end watchdog;

architecture watchdog of watchdog is
  signal counter : unsigned (bits downto 0) := (others => '0');
  signal lastI : std_logic;
begin
  Ok <= counter(bits);
  process
  begin
    wait until rising_edge(clk);
    if lastI /= I then
      counter <= (bits => '1', others => '0');
    elsif counter(bits) = '1' then
      counter <= counter + 1;
    end if;
    lastI <= I;
  end process;
end watchdog;