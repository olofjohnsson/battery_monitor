### Good to know
// Enable the DC/DC converter<br>
nrf_power_dcdcen_set(NRF_POWER, true);<br>
// Set REGOUT0 to 3.3V<br>
NRF_UICR->REGOUT0 = UICR_REGOUT0_VOUT_3V3;

Relay cannot handle stopping and changing directio in one go. It needs time to stop before rotation in the other direction can be triggered.

### ToDo
Play spookie sound a bit earlier when PIR detection happens.