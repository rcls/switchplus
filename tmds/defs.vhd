library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package defs is

  subtype byte_t is unsigned (7 downto 0);
  subtype dec_t is unsigned (9 downto 0);
  subtype nibble_t is unsigned (3 downto 0);

  subtype u28 is unsigned (27 downto 0);

  attribute keep_hierarchy : string;

  function bi(b : std_logic) return natural is
  begin if b = '1' then return 1; else return 0; end if; end bi;
  function bb(b : boolean) return std_logic is
  begin if b then return '1'; else return '0'; end if; end bb;

  -- bits 14..23 are mostly 0, with signed excess in 24..27
  -- bits 0..9 are mostly 1, with signed excess in 10..13
  function encode_byte(D : byte_t) return u28 is
    variable q_m : dec_t;
    variable qmc : integer;
    variable out_p, out_n : unsigned(13 downto 0);
  begin
    q_m(0) := D(0);
    q_m(8) := bb(bi(d(1)) + bi(d(2)) + bi(d(3)) + bi(d(4)) +
                 bi(d(5)) + bi(d(6)) + bi(d(7)) < 4);
    for i in 1 to 7 loop
      q_m(i) := not q_m(i-1) xor d(i-1) xor q_m(8);
    end loop;
    q_m(9) := '0';

    qmc := bi(q_m(0)) + bi(q_m(1)) + bi(q_m(2)) + bi(q_m(3)) +
           bi(q_m(4)) + bi(q_m(5)) + bi(q_m(6)) + bi(q_m(7));
    out_p := to_unsigned(bi(q_m(8)) + qmc - 5, 4) & q_m;
    out_n := to_unsigned(bi(q_m(8)) - qmc + 4, 4) & (q_m xor "1011111111");

    if qmc < 4 then
      return out_p & out_n;
    elsif qmc > 4 then
      return out_n & out_p;
    elsif q_m(8) = '0' then
      return out_n & out_n;
    else
      return out_p * out_p;
    end if;
  end encode_byte;

  type encode_rom_t is array (0 to 255) of u28;
  function encode_rom_f(dummy : integer) return encode_rom_t is
    variable result : encode_rom_t;
  begin
    for i in 0 to 255 loop
      result(i) := encode_byte(to_unsigned(i, 8));
    end loop;
    return result;
  end encode_rom_f;

end defs;
