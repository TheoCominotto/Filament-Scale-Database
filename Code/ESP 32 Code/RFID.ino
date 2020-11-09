
bool RFIDpresent() //When this function is called, it checks whether an RFID tag is present. If so, it saves the UID and returns 'true'
{
  //Serial.println("RFID check");
  // Look for new cards
  if (  mfrc522.PICC_IsNewCardPresent())
  {
    // Select one of the cards
    if (mfrc522.PICC_ReadCardSerial())
    {

      char str[32] = "";
      array_to_string(mfrc522.uid.uidByte, 4, UID); //Insert (byte array, length, char array for output)
      mfrc522.PICC_HaltA();


      //  UID.replace(' ', '_');
      Serial.println(UID);
      //sendData(UID, W);

      return true;
    }
  }

  return false;
}

//-------------------------------

void array_to_string(byte array[], unsigned int len, char buffer[]) //Converts the UID byte array to a char array
{
  for (unsigned int i = 0; i < len; i++)
  {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i * 2 + 0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
    buffer[i * 2 + 1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  buffer[len * 2] = '\0';
}
