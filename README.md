Motorized Guitar Tuner:  

Current Components:  
STM32 Nucleo G474RE, NEMA-14 stepper motor, 2.4" ST7789VI LCD, TMC2209 Motor Driver, MCP6001 Low Power Op-Amp, CEB-2021-L-100 Piezo  

Operation:  
- On startup/reset, user selects guitar tuning  
- Once user selects tuning, tuning mode is entered  
- For all 6 strings, the user puts the stepper motor with tuning peg adapter onto the tuning peg, then plucks the string  
- The device determines if correction is needed, then turns the peg until the string is in tune  
- Once tuning of all strings is done, user may select another tuning and tune again  

Implementation Details: 
- Piezo is sampled with ADC and transfered to memory with DMA  
- Frequency spectrum calculated with KissFFT, along with peak bin interpolation for accurate frequency measurements (bin spacing is around 3.9Hz so interpolation helps estimate within a couple of Hz)

- LCD interfaced with SPI; created custom lightweight renderer allowing glyph/text display  
- UI draws are handled with dirty bits within the state machine, to avoid redrawing unchanged UI elements

- System control also handled with separate state machine

Pics:  
<img width="436" height="300" alt="image" src="https://github.com/user-attachments/assets/f021af29-bde7-46ff-9277-4f2b53856aa2" /><img width="436" height="300" alt="WhatsApp Image 2026-10-02 at 2 59 52 PM" src="https://github.com/user-attachments/assets/c4da6898-6643-4ab8-8640-af9564948730" />  
Tuning Selection Screen  

<img width="436" height="300" alt="image" src="https://github.com/user-attachments/assets/fcfbccc1-a847-4bcf-85c5-0001a1f97a03" /><img width="436" height="300" alt="image" src="https://github.com/user-attachments/assets/e9bb8b3c-a241-49ee-be24-073b4d30a8b0" />  
String Tuning Screen  
  
<img width="436" height="300" alt="image" src="https://github.com/user-attachments/assets/84f65645-9129-4507-bb37-c07610984064" />    
Tuning Complete Screen  
  
<img width="300" height="436" alt="image" src="https://github.com/user-attachments/assets/775c6f18-d911-4963-9bd4-3ab813ad09fb" />  
3d-printed Tuning Peg Stepper Motor Adapter  
