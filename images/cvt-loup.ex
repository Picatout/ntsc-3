-- Date: 2013-08-01
-- Description: script OpenEuphoria pour convertir fichier loup.txt qui est lui-même le data extrait de loup.bmp
--              en image noir et blanc avec dithering utilisant l'algorithme de Floyd-Steinberg
--              Crée le fichier loup.h qui est un tableau 'C' qui est inclu dans le programme principal 
-- Auteur: Jaccques Deschênes
-- REF: http://en.wikipedia.org/wiki/Floyd%E2%80%93Steinberg_dithering

include get.e

sequence input
input="loup.txt"
sequence output
output ="..\\loup.h"
integer fo,fi

fo=open(output,"w")
fi=open(input,"r")
sequence original
original=get(fi)
if (original[1]=GET_SUCCESS) then
   if (atom(original[2])) then
      original = {original[2]}
   else
      original=original[2]
   end if
else
   abort(0)
end if

integer new_width
new_width = length(original[1])/8
if (remainder(length(original[1]),8)) then
  new_width +=1
end if

sequence b_w
b_w= repeat(repeat(0,new_width),length(original))
     
close(fi)


procedure write_pixel(integer v, integer x, integer y)
   integer byte, bit
   byte = floor((x-1)/8)
   bit = 7-remainder(x-1,8)
   b_w[y][byte+1] += v*power(2,bit)
end procedure


atom new_pixel, old_pixel
sequence line
atom error
for y=1 to length(original) do
   for x=1 to length(original[y]) do
       old_pixel=original[y][x]
       if old_pixel>127 then
           new_pixel=1
           error = old_pixel-255
       else
           new_pixel=0
           error = old_pixel
       end if
       --error=old_pixel-new_pixel
       write_pixel(new_pixel,x,y)
       if x<length(original[y]) then
               original[y][x+1]= original[y][x+1]+ 7/16*error
       end if
       if x>1 and y < length(original) then
           original[y+1][x-1]= original[y+1][x-1]+ 3/16*error
       end if
       if y<length(original) then
           original[y+1][x]= original[y+1][x]+ 5/16*error
       end if
       if y<length(original) and x<length(original[y]) then
          original[y+1][x+1]= original[y+1][x+1]+ 1/16*error
       end if
   end for
end for

puts(fo,"const char loup[][22]={\n")
for y=1 to length(b_w) do
   puts(fo,"{")
   for x=1 to length(b_w[y]) do
       if x<length(b_w[y]) then
           printf(fo,"0x%02x, ",b_w[y][x])
       else
           printf(fo,"0x%02x},\n",b_w[y][x])
       end if
   end for
end for
puts(fo,"};\n")
close(fo)

