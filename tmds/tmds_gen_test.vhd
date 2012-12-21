library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity tmds_gen_test is
  port (Rp, Rn, Gp, Gn, Bp, Bn, Cp, Cn : out std_logic);
end tmds_gen_test;
architecture tmds_gen_test of tmds_gen_test is
  signal clk : std_logic;
begin
  tg : entity work.tmds_gen port map (
    Rp=> Rp, Rn=> Rn, Gp=> Gp, Gn=> Gn, Bp=> Bp, Bn=> Bn, Cp=> Cp, Cn=> Cn,
    clkin100=> clk);
  process
  begin
    clk <= '0';
    wait for 5 ns;
    clk <= '1';
    wait for 5 ns;
  end process;
end tmds_gen_test;
