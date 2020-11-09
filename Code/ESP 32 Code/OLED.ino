

void OLED_LoadingBar(int *prog)
{
  u8g2.clearBuffer();

  int barHeight = 40;
  int barWidth = 10;

  //Serial.println(prog[0]);
  if (prog[0] <= period)
  {
    int currSize =  minSize + prog[0];
    u8g2.drawBox((u8g2.getDisplayWidth() / 2) - (currSize / 2), barHeight, currSize, barWidth);
  }
  else if (prog[0] > maxSize - minSize && prog[0] < (period * 2))
  {
    int currSize =   maxSize - (prog[0] - period);
    u8g2.drawBox((u8g2.getDisplayWidth() / 2) - (currSize / 2), barHeight, currSize, barWidth);
  }
  else
  {
    prog[0] = 0;
  }

  prog[0] += 4;



  u8g2.setCursor((u8g2.getDisplayWidth() / 2) - (u8g2.getStrWidth("Connecting to") / 2), 14);
  u8g2.print("Connecting to");
  u8g2.setCursor((u8g2.getDisplayWidth() / 2) - (u8g2.getStrWidth(ssid) / 2), 26);
  u8g2.print(ssid);

  u8g2.sendBuffer();
}

void OLED_print(String message, int place)
{
  char charBuf[50];
  message.toCharArray(charBuf, 50);

  u8g2.setCursor((u8g2.getDisplayWidth() / 2) - (u8g2.getStrWidth(charBuf) / 2), place);
  u8g2.print(charBuf);
}
