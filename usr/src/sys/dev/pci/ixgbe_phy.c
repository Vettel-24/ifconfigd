/*	$OpenBSD: ixgbe_phy.c,v 1.5 2010/09/21 00:29:29 claudio Exp $	*/

/******************************************************************************

  Copyright (c) 2001-2009, Intel Corporation 
  All rights reserved.
  
  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions are met:
  
   1. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
  
   2. Redistributions in binary form must reproduce the above copyright 
      notice, this list of conditions and the following disclaimer in the 
      documentation and/or other materials provided with the distribution.
  
   3. Neither the name of the Intel Corporation nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.
  
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/
/*$FreeBSD: src/sys/dev/ixgbe/ixgbe_phy.c,v 1.9 2009/12/07 21:30:54 jfv Exp $*/

#include <dev/pci/ixgbe.h>

void ixgbe_i2c_start(struct ixgbe_hw *hw);
void ixgbe_i2c_stop(struct ixgbe_hw *hw);
int32_t ixgbe_clock_in_i2c_byte(struct ixgbe_hw *hw, uint8_t *data);
int32_t ixgbe_clock_out_i2c_byte(struct ixgbe_hw *hw, uint8_t data);
int32_t ixgbe_get_i2c_ack(struct ixgbe_hw *hw);
int32_t ixgbe_clock_in_i2c_bit(struct ixgbe_hw *hw, int *data);
int32_t ixgbe_clock_out_i2c_bit(struct ixgbe_hw *hw, int data);
int32_t ixgbe_raise_i2c_clk(struct ixgbe_hw *hw, uint32_t *i2cctl);
void ixgbe_lower_i2c_clk(struct ixgbe_hw *hw, uint32_t *i2cctl);
int32_t ixgbe_set_i2c_data(struct ixgbe_hw *hw, uint32_t *i2cctl, int data);
int ixgbe_get_i2c_data(uint32_t *i2cctl);
void ixgbe_i2c_bus_clear(struct ixgbe_hw *hw);

/**
 *  ixgbe_init_phy_ops_generic - Inits PHY function ptrs
 *  @hw: pointer to the hardware structure
 *
 *  Initialize the function pointers.
 **/
int32_t ixgbe_init_phy_ops_generic(struct ixgbe_hw *hw)
{
	struct ixgbe_phy_info *phy = &hw->phy;

	/* PHY */
	phy->ops.identify = &ixgbe_identify_phy_generic;
	phy->ops.reset = &ixgbe_reset_phy_generic;
	phy->ops.read_reg = &ixgbe_read_phy_reg_generic;
	phy->ops.write_reg = &ixgbe_write_phy_reg_generic;
	phy->ops.setup_link = &ixgbe_setup_phy_link_generic;
	phy->ops.setup_link_speed = &ixgbe_setup_phy_link_speed_generic;
	phy->ops.check_link = NULL;
	phy->ops.get_firmware_version = ixgbe_get_phy_firmware_version_generic;
	phy->ops.read_i2c_byte = &ixgbe_read_i2c_byte_generic;
	phy->ops.write_i2c_byte = &ixgbe_write_i2c_byte_generic;
	phy->ops.read_i2c_eeprom = &ixgbe_read_i2c_eeprom_generic;
	phy->ops.write_i2c_eeprom = &ixgbe_write_i2c_eeprom_generic;
	phy->ops.i2c_bus_clear = &ixgbe_i2c_bus_clear;
	phy->ops.identify_sfp = &ixgbe_identify_sfp_module_generic;
	phy->sfp_type = ixgbe_sfp_type_unknown;

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_identify_phy_generic - Get physical layer module
 *  @hw: pointer to hardware structure
 *
 *  Determines the physical layer module found on the current adapter.
 **/
int32_t ixgbe_identify_phy_generic(struct ixgbe_hw *hw)
{
	int32_t status = IXGBE_ERR_PHY_ADDR_INVALID;
	uint32_t phy_addr;
	uint16_t ext_ability = 0;

	if (hw->phy.type == ixgbe_phy_unknown) {
		for (phy_addr = 0; phy_addr < IXGBE_MAX_PHY_ADDR; phy_addr++) {
			if (ixgbe_validate_phy_addr(hw, phy_addr)) {
				hw->phy.addr = phy_addr;
				ixgbe_get_phy_id(hw);
				hw->phy.type =
				        ixgbe_get_phy_type_from_id(hw->phy.id);

				if (hw->phy.type == ixgbe_phy_unknown) {
					hw->phy.ops.read_reg(hw,
						  IXGBE_MDIO_PHY_EXT_ABILITY,
					          IXGBE_MDIO_PMA_PMD_DEV_TYPE,
					          &ext_ability);
					if (ext_ability &
					    IXGBE_MDIO_PHY_10GBASET_ABILITY ||
					    ext_ability &
					    IXGBE_MDIO_PHY_1000BASET_ABILITY)
						hw->phy.type =
						         ixgbe_phy_cu_unknown;
					else
						hw->phy.type =
						         ixgbe_phy_generic;
				}

				status = IXGBE_SUCCESS;
				break;
			}
		}
		if (status != IXGBE_SUCCESS)
			hw->phy.addr = 0;
	} else {
		status = IXGBE_SUCCESS;
	}

	return status;
}

/**
 *  ixgbe_validate_phy_addr - Determines phy address is valid
 *  @hw: pointer to hardware structure
 *
 **/
int ixgbe_validate_phy_addr(struct ixgbe_hw *hw, uint32_t phy_addr)
{
	uint16_t phy_id = 0;
	int valid = FALSE;

	hw->phy.addr = phy_addr;
	hw->phy.ops.read_reg(hw, IXGBE_MDIO_PHY_ID_HIGH,
	                     IXGBE_MDIO_PMA_PMD_DEV_TYPE, &phy_id);

	if (phy_id != 0xFFFF && phy_id != 0x0)
		valid = TRUE;

	return valid;
}

/**
 *  ixgbe_get_phy_id - Get the phy type
 *  @hw: pointer to hardware structure
 *
 **/
int32_t ixgbe_get_phy_id(struct ixgbe_hw *hw)
{
	uint32_t status;
	uint16_t phy_id_high = 0;
	uint16_t phy_id_low = 0;

	status = hw->phy.ops.read_reg(hw, IXGBE_MDIO_PHY_ID_HIGH,
	                              IXGBE_MDIO_PMA_PMD_DEV_TYPE,
	                              &phy_id_high);

	if (status == IXGBE_SUCCESS) {
		hw->phy.id = (uint32_t)(phy_id_high << 16);
		status = hw->phy.ops.read_reg(hw, IXGBE_MDIO_PHY_ID_LOW,
		                              IXGBE_MDIO_PMA_PMD_DEV_TYPE,
		                              &phy_id_low);
		hw->phy.id |= (uint32_t)(phy_id_low & IXGBE_PHY_REVISION_MASK);
		hw->phy.revision = (uint32_t)(phy_id_low & ~IXGBE_PHY_REVISION_MASK);
	}
	return status;
}

/**
 *  ixgbe_get_phy_type_from_id - Get the phy type
 *  @hw: pointer to hardware structure
 *
 **/
enum ixgbe_phy_type ixgbe_get_phy_type_from_id(uint32_t phy_id)
{
	enum ixgbe_phy_type phy_type;

	switch (phy_id) {
	case TN1010_PHY_ID:
		phy_type = ixgbe_phy_tn;
		break;
	case AQ1002_PHY_ID:
		phy_type = ixgbe_phy_aq;
		break;
	case QT2022_PHY_ID:
		phy_type = ixgbe_phy_qt;
		break;
	case ATH_PHY_ID:
		phy_type = ixgbe_phy_nl;
		break;
	default:
		phy_type = ixgbe_phy_unknown;
		break;
	}

	DEBUGOUT1("phy type found is %d\n", phy_type);
	return phy_type;
}

/**
 *  ixgbe_reset_phy_generic - Performs a PHY reset
 *  @hw: pointer to hardware structure
 **/
int32_t ixgbe_reset_phy_generic(struct ixgbe_hw *hw)
{
	int32_t i;
	uint16_t ctrl = 0;
	int32_t status = IXGBE_SUCCESS;

	if (hw->phy.type == ixgbe_phy_unknown)
		status = ixgbe_identify_phy_generic(hw);

	if (status != IXGBE_SUCCESS || hw->phy.type == ixgbe_phy_none)
		goto out;

	/*
	 * Perform soft PHY reset to the PHY_XS.
	 * This will cause a soft reset to the PHY
	 */
	hw->phy.ops.write_reg(hw, IXGBE_MDIO_PHY_XS_CONTROL,
	                      IXGBE_MDIO_PHY_XS_DEV_TYPE,
	                      IXGBE_MDIO_PHY_XS_RESET);

	/* Poll for reset bit to self-clear indicating reset is complete */
	for (i = 0; i < 500; i++) {
		msec_delay(1);
		hw->phy.ops.read_reg(hw, IXGBE_MDIO_PHY_XS_CONTROL,
		                     IXGBE_MDIO_PHY_XS_DEV_TYPE, &ctrl);
		if (!(ctrl & IXGBE_MDIO_PHY_XS_RESET))
			break;
	}

	if (ctrl & IXGBE_MDIO_PHY_XS_RESET) {
		status = IXGBE_ERR_RESET_FAILED;
		DEBUGOUT("PHY reset polling failed to complete.\n");
	}

out:
	return status;
}

/**
 *  ixgbe_read_phy_reg_generic - Reads a value from a specified PHY register
 *  @hw: pointer to hardware structure
 *  @reg_addr: 32 bit address of PHY register to read
 *  @phy_data: Pointer to read data from PHY register
 **/
int32_t ixgbe_read_phy_reg_generic(struct ixgbe_hw *hw, uint32_t reg_addr,
                               uint32_t device_type, uint16_t *phy_data)
{
	uint32_t command;
	uint32_t i;
	uint32_t data;
	int32_t status = IXGBE_SUCCESS;
	uint16_t gssr;

	if (IXGBE_READ_REG(hw, IXGBE_STATUS) & IXGBE_STATUS_LAN_ID_1)
		gssr = IXGBE_GSSR_PHY1_SM;
	else
		gssr = IXGBE_GSSR_PHY0_SM;

	if (ixgbe_acquire_swfw_sync(hw, gssr) != IXGBE_SUCCESS)
		status = IXGBE_ERR_SWFW_SYNC;

	if (status == IXGBE_SUCCESS) {
		/* Setup and write the address cycle command */
		command = ((reg_addr << IXGBE_MSCA_NP_ADDR_SHIFT)  |
		           (device_type << IXGBE_MSCA_DEV_TYPE_SHIFT) |
		           (hw->phy.addr << IXGBE_MSCA_PHY_ADDR_SHIFT) |
		           (IXGBE_MSCA_ADDR_CYCLE | IXGBE_MSCA_MDI_COMMAND));

		IXGBE_WRITE_REG(hw, IXGBE_MSCA, command);

		/*
		 * Check every 10 usec to see if the address cycle completed.
		 * The MDI Command bit will clear when the operation is
		 * complete
		 */
		for (i = 0; i < IXGBE_MDIO_COMMAND_TIMEOUT; i++) {
			usec_delay(10);

			command = IXGBE_READ_REG(hw, IXGBE_MSCA);

			if ((command & IXGBE_MSCA_MDI_COMMAND) == 0) {
				break;
			}
		}

		if ((command & IXGBE_MSCA_MDI_COMMAND) != 0) {
			DEBUGOUT("PHY address command did not complete.\n");
			status = IXGBE_ERR_PHY;
		}

		if (status == IXGBE_SUCCESS) {
			/*
			 * Address cycle complete, setup and write the read
			 * command
			 */
			command = ((reg_addr << IXGBE_MSCA_NP_ADDR_SHIFT)  |
			           (device_type << IXGBE_MSCA_DEV_TYPE_SHIFT) |
			           (hw->phy.addr << IXGBE_MSCA_PHY_ADDR_SHIFT) |
			           (IXGBE_MSCA_READ | IXGBE_MSCA_MDI_COMMAND));

			IXGBE_WRITE_REG(hw, IXGBE_MSCA, command);

			/*
			 * Check every 10 usec to see if the address cycle
			 * completed. The MDI Command bit will clear when the
			 * operation is complete
			 */
			for (i = 0; i < IXGBE_MDIO_COMMAND_TIMEOUT; i++) {
				usec_delay(10);

				command = IXGBE_READ_REG(hw, IXGBE_MSCA);

				if ((command & IXGBE_MSCA_MDI_COMMAND) == 0)
					break;
			}

			if ((command & IXGBE_MSCA_MDI_COMMAND) != 0) {
				DEBUGOUT("PHY read command didn't complete\n");
				status = IXGBE_ERR_PHY;
			} else {
				/*
				 * Read operation is complete.  Get the data
				 * from MSRWD
				 */
				data = IXGBE_READ_REG(hw, IXGBE_MSRWD);
				data >>= IXGBE_MSRWD_READ_DATA_SHIFT;
				*phy_data = (uint16_t)(data);
			}
		}

		ixgbe_release_swfw_sync(hw, gssr);
	}

	return status;
}

/**
 *  ixgbe_write_phy_reg_generic - Writes a value to specified PHY register
 *  @hw: pointer to hardware structure
 *  @reg_addr: 32 bit PHY register to write
 *  @device_type: 5 bit device type
 *  @phy_data: Data to write to the PHY register
 **/
int32_t ixgbe_write_phy_reg_generic(struct ixgbe_hw *hw, uint32_t reg_addr,
                                uint32_t device_type, uint16_t phy_data)
{
	uint32_t command;
	uint32_t i;
	int32_t status = IXGBE_SUCCESS;
	uint16_t gssr;

	if (IXGBE_READ_REG(hw, IXGBE_STATUS) & IXGBE_STATUS_LAN_ID_1)
		gssr = IXGBE_GSSR_PHY1_SM;
	else
		gssr = IXGBE_GSSR_PHY0_SM;

	if (ixgbe_acquire_swfw_sync(hw, gssr) != IXGBE_SUCCESS)
		status = IXGBE_ERR_SWFW_SYNC;

	if (status == IXGBE_SUCCESS) {
		/* Put the data in the MDI single read and write data register*/
		IXGBE_WRITE_REG(hw, IXGBE_MSRWD, (uint32_t)phy_data);

		/* Setup and write the address cycle command */
		command = ((reg_addr << IXGBE_MSCA_NP_ADDR_SHIFT)  |
		           (device_type << IXGBE_MSCA_DEV_TYPE_SHIFT) |
		           (hw->phy.addr << IXGBE_MSCA_PHY_ADDR_SHIFT) |
		           (IXGBE_MSCA_ADDR_CYCLE | IXGBE_MSCA_MDI_COMMAND));

		IXGBE_WRITE_REG(hw, IXGBE_MSCA, command);

		/*
		 * Check every 10 usec to see if the address cycle completed.
		 * The MDI Command bit will clear when the operation is
		 * complete
		 */
		for (i = 0; i < IXGBE_MDIO_COMMAND_TIMEOUT; i++) {
			usec_delay(10);

			command = IXGBE_READ_REG(hw, IXGBE_MSCA);

			if ((command & IXGBE_MSCA_MDI_COMMAND) == 0)
				break;
		}

		if ((command & IXGBE_MSCA_MDI_COMMAND) != 0) {
			DEBUGOUT("PHY address cmd didn't complete\n");
			status = IXGBE_ERR_PHY;
		}

		if (status == IXGBE_SUCCESS) {
			/*
			 * Address cycle complete, setup and write the write
			 * command
			 */
			command = ((reg_addr << IXGBE_MSCA_NP_ADDR_SHIFT)  |
			           (device_type << IXGBE_MSCA_DEV_TYPE_SHIFT) |
			           (hw->phy.addr << IXGBE_MSCA_PHY_ADDR_SHIFT) |
			           (IXGBE_MSCA_WRITE | IXGBE_MSCA_MDI_COMMAND));

			IXGBE_WRITE_REG(hw, IXGBE_MSCA, command);

			/*
			 * Check every 10 usec to see if the address cycle
			 * completed. The MDI Command bit will clear when the
			 * operation is complete
			 */
			for (i = 0; i < IXGBE_MDIO_COMMAND_TIMEOUT; i++) {
				usec_delay(10);

				command = IXGBE_READ_REG(hw, IXGBE_MSCA);

				if ((command & IXGBE_MSCA_MDI_COMMAND) == 0)
					break;
			}

			if ((command & IXGBE_MSCA_MDI_COMMAND) != 0) {
				DEBUGOUT("PHY address cmd didn't complete\n");
				status = IXGBE_ERR_PHY;
			}
		}

		ixgbe_release_swfw_sync(hw, gssr);
	}

	return status;
}

/**
 *	ixgbe_setup_phy_link_generic - Set and restart autoneg
 *	@hw: pointer to hardware structure
 *
 *	Restart autonegotiation and PHY and waits for completion.
 **/
int32_t ixgbe_setup_phy_link_generic(struct ixgbe_hw *hw)
{
	int32_t status = IXGBE_SUCCESS;
	uint32_t time_out;
	uint32_t max_time_out = 10;
	uint16_t autoneg_reg = IXGBE_MII_AUTONEG_REG;
	int autoneg = 0;
	ixgbe_link_speed speed;


	ixgbe_get_copper_link_capabilities_generic(hw, &speed, &autoneg);

	if (speed & IXGBE_LINK_SPEED_10GB_FULL) {
		/* Set or unset auto-negotiation 10G advertisement */
		hw->phy.ops.read_reg(hw, IXGBE_MII_10GBASE_T_AUTONEG_CTRL_REG,
		                     IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
	                             &autoneg_reg);

		autoneg_reg &= ~IXGBE_MII_10GBASE_T_ADVERTISE;
		if (hw->phy.autoneg_advertised & IXGBE_LINK_SPEED_10GB_FULL)
			autoneg_reg |= IXGBE_MII_10GBASE_T_ADVERTISE;

		hw->phy.ops.write_reg(hw, IXGBE_MII_10GBASE_T_AUTONEG_CTRL_REG,
		                      IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
		                      autoneg_reg);
	}

	if (speed & IXGBE_LINK_SPEED_1GB_FULL) {
		/* Set or unset auto-negotiation 1G advertisement */
		hw->phy.ops.read_reg(hw,
		                     IXGBE_MII_AUTONEG_VENDOR_PROVISION_1_REG,
		                     IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
		                     &autoneg_reg);

		autoneg_reg &= ~IXGBE_MII_1GBASE_T_ADVERTISE;
		if (hw->phy.autoneg_advertised & IXGBE_LINK_SPEED_1GB_FULL)
			autoneg_reg |= IXGBE_MII_1GBASE_T_ADVERTISE;

		hw->phy.ops.write_reg(hw,
		                      IXGBE_MII_AUTONEG_VENDOR_PROVISION_1_REG,
		                      IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
		                      autoneg_reg);
	}

	if (speed & IXGBE_LINK_SPEED_100_FULL) {
		/* Set or unset auto-negotiation 100M advertisement */
		hw->phy.ops.read_reg(hw, IXGBE_MII_AUTONEG_ADVERTISE_REG,
		                     IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
		                     &autoneg_reg);

		autoneg_reg &= ~IXGBE_MII_100BASE_T_ADVERTISE;
		if (hw->phy.autoneg_advertised & IXGBE_LINK_SPEED_100_FULL)
			autoneg_reg |= IXGBE_MII_100BASE_T_ADVERTISE;

		hw->phy.ops.write_reg(hw, IXGBE_MII_AUTONEG_ADVERTISE_REG,
		                      IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
		                      autoneg_reg);
	}

	/* Restart PHY autonegotiation and wait for completion */
	hw->phy.ops.read_reg(hw, IXGBE_MDIO_AUTO_NEG_CONTROL,
	                     IXGBE_MDIO_AUTO_NEG_DEV_TYPE, &autoneg_reg);

	autoneg_reg |= IXGBE_MII_RESTART;

	hw->phy.ops.write_reg(hw, IXGBE_MDIO_AUTO_NEG_CONTROL,
	                      IXGBE_MDIO_AUTO_NEG_DEV_TYPE, autoneg_reg);

	/* Wait for autonegotiation to finish */
	for (time_out = 0; time_out < max_time_out; time_out++) {
		usec_delay(10);
		/* Restart PHY autonegotiation and wait for completion */
		status = hw->phy.ops.read_reg(hw, IXGBE_MDIO_AUTO_NEG_STATUS,
		                              IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
		                              &autoneg_reg);

		autoneg_reg &= IXGBE_MII_AUTONEG_COMPLETE;
		if (autoneg_reg == IXGBE_MII_AUTONEG_COMPLETE)
			break;
	}

	if (time_out == max_time_out)
		status = IXGBE_ERR_LINK_SETUP;

	return status;
}

/**
 *  ixgbe_setup_phy_link_speed_generic - Sets the auto advertised capabilities
 *  @hw: pointer to hardware structure
 *  @speed: new link speed
 *  @autoneg: TRUE if autonegotiation enabled
 **/
int32_t ixgbe_setup_phy_link_speed_generic(struct ixgbe_hw *hw,
                                       ixgbe_link_speed speed,
                                       int autoneg,
                                       int autoneg_wait_to_complete)
{
	UNREFERENCED_PARAMETER(autoneg);
	UNREFERENCED_PARAMETER(autoneg_wait_to_complete);

	/*
	 * Clear autoneg_advertised and set new values based on input link
	 * speed.
	 */
	hw->phy.autoneg_advertised = 0;

	if (speed & IXGBE_LINK_SPEED_10GB_FULL)
		hw->phy.autoneg_advertised |= IXGBE_LINK_SPEED_10GB_FULL;

	if (speed & IXGBE_LINK_SPEED_1GB_FULL)
		hw->phy.autoneg_advertised |= IXGBE_LINK_SPEED_1GB_FULL;

	if (speed & IXGBE_LINK_SPEED_100_FULL)
		hw->phy.autoneg_advertised |= IXGBE_LINK_SPEED_100_FULL;

	/* Setup link based on the new speed settings */
	hw->phy.ops.setup_link(hw);

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_get_copper_link_capabilities_generic - Determines link capabilities
 *  @hw: pointer to hardware structure
 *  @speed: pointer to link speed
 *  @autoneg: boolean auto-negotiation value
 *
 *  Determines the link capabilities by reading the AUTOC register.
 **/
int32_t ixgbe_get_copper_link_capabilities_generic(struct ixgbe_hw *hw,
                                             ixgbe_link_speed *speed,
                                             int *autoneg)
{
	int32_t status = IXGBE_ERR_LINK_SETUP;
	uint16_t speed_ability;

	*speed = 0;
	*autoneg = TRUE;

	status = hw->phy.ops.read_reg(hw, IXGBE_MDIO_PHY_SPEED_ABILITY,
	                              IXGBE_MDIO_PMA_PMD_DEV_TYPE,
	                              &speed_ability);

	if (status == IXGBE_SUCCESS) {
		if (speed_ability & IXGBE_MDIO_PHY_SPEED_10G)
			*speed |= IXGBE_LINK_SPEED_10GB_FULL;
		if (speed_ability & IXGBE_MDIO_PHY_SPEED_1G)
			*speed |= IXGBE_LINK_SPEED_1GB_FULL;
		if (speed_ability & IXGBE_MDIO_PHY_SPEED_100M)
			*speed |= IXGBE_LINK_SPEED_100_FULL;
	}

	return status;
}

/**
 *  ixgbe_check_phy_link_tnx - Determine link and speed status
 *  @hw: pointer to hardware structure
 *
 *  Reads the VS1 register to determine if link is up and the current speed for
 *  the PHY.
 **/
int32_t ixgbe_check_phy_link_tnx(struct ixgbe_hw *hw, ixgbe_link_speed *speed,
                             int *link_up)
{
	int32_t status = IXGBE_SUCCESS;
	uint32_t time_out;
	uint32_t max_time_out = 10;
	uint16_t phy_link = 0;
	uint16_t phy_speed = 0;
	uint16_t phy_data = 0;

	/* Initialize speed and link to default case */
	*link_up = FALSE;
	*speed = IXGBE_LINK_SPEED_10GB_FULL;

	/*
	 * Check current speed and link status of the PHY register.
	 * This is a vendor specific register and may have to
	 * be changed for other copper PHYs.
	 */
	for (time_out = 0; time_out < max_time_out; time_out++) {
		usec_delay(10);
		status = hw->phy.ops.read_reg(hw,
		                        IXGBE_MDIO_VENDOR_SPECIFIC_1_STATUS,
		                        IXGBE_MDIO_VENDOR_SPECIFIC_1_DEV_TYPE,
		                        &phy_data);
		phy_link = phy_data &
		           IXGBE_MDIO_VENDOR_SPECIFIC_1_LINK_STATUS;
		phy_speed = phy_data &
		            IXGBE_MDIO_VENDOR_SPECIFIC_1_SPEED_STATUS;
		if (phy_link == IXGBE_MDIO_VENDOR_SPECIFIC_1_LINK_STATUS) {
			*link_up = TRUE;
			if (phy_speed ==
			    IXGBE_MDIO_VENDOR_SPECIFIC_1_SPEED_STATUS)
				*speed = IXGBE_LINK_SPEED_1GB_FULL;
			break;
		}
	}

	return status;
}

/**
 *	ixgbe_setup_phy_link_tnx - Set and restart autoneg
 *	@hw: pointer to hardware structure
 *
 *	Restart autonegotiation and PHY and waits for completion.
 **/
int32_t ixgbe_setup_phy_link_tnx(struct ixgbe_hw *hw)
{
	int32_t status = IXGBE_SUCCESS;
	uint32_t time_out;
	uint32_t max_time_out = 10;
	uint16_t autoneg_reg = IXGBE_MII_AUTONEG_REG;
	int autoneg = FALSE;
	ixgbe_link_speed speed;

	ixgbe_get_copper_link_capabilities_generic(hw, &speed, &autoneg);

	if (speed & IXGBE_LINK_SPEED_10GB_FULL) {
		/* Set or unset auto-negotiation 10G advertisement */
		hw->phy.ops.read_reg(hw, IXGBE_MII_10GBASE_T_AUTONEG_CTRL_REG,
		                     IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
		                     &autoneg_reg);

		autoneg_reg &= ~IXGBE_MII_10GBASE_T_ADVERTISE;
		if (hw->phy.autoneg_advertised & IXGBE_LINK_SPEED_10GB_FULL)
			autoneg_reg |= IXGBE_MII_10GBASE_T_ADVERTISE;

		hw->phy.ops.write_reg(hw, IXGBE_MII_10GBASE_T_AUTONEG_CTRL_REG,
		                      IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
		                      autoneg_reg);
	}

	if (speed & IXGBE_LINK_SPEED_1GB_FULL) {
		/* Set or unset auto-negotiation 1G advertisement */
		hw->phy.ops.read_reg(hw, IXGBE_MII_AUTONEG_XNP_TX_REG,
		                     IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
		                     &autoneg_reg);

		autoneg_reg &= ~IXGBE_MII_1GBASE_T_ADVERTISE_XNP_TX;
		if (hw->phy.autoneg_advertised & IXGBE_LINK_SPEED_1GB_FULL)
			autoneg_reg |= IXGBE_MII_1GBASE_T_ADVERTISE_XNP_TX;

		hw->phy.ops.write_reg(hw, IXGBE_MII_AUTONEG_XNP_TX_REG,
		                      IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
		                      autoneg_reg);
	}

	if (speed & IXGBE_LINK_SPEED_100_FULL) {
		/* Set or unset auto-negotiation 100M advertisement */
		hw->phy.ops.read_reg(hw, IXGBE_MII_AUTONEG_ADVERTISE_REG,
		                     IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
		                     &autoneg_reg);

		autoneg_reg &= ~IXGBE_MII_100BASE_T_ADVERTISE;
		if (hw->phy.autoneg_advertised & IXGBE_LINK_SPEED_100_FULL)
			autoneg_reg |= IXGBE_MII_100BASE_T_ADVERTISE;

		hw->phy.ops.write_reg(hw, IXGBE_MII_AUTONEG_ADVERTISE_REG,
		                      IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
		                      autoneg_reg);
	}

	/* Restart PHY autonegotiation and wait for completion */
	hw->phy.ops.read_reg(hw, IXGBE_MDIO_AUTO_NEG_CONTROL,
	                     IXGBE_MDIO_AUTO_NEG_DEV_TYPE, &autoneg_reg);

	autoneg_reg |= IXGBE_MII_RESTART;

	hw->phy.ops.write_reg(hw, IXGBE_MDIO_AUTO_NEG_CONTROL,
	                      IXGBE_MDIO_AUTO_NEG_DEV_TYPE, autoneg_reg);

	/* Wait for autonegotiation to finish */
	for (time_out = 0; time_out < max_time_out; time_out++) {
		usec_delay(10);
		/* Restart PHY autonegotiation and wait for completion */
		status = hw->phy.ops.read_reg(hw, IXGBE_MDIO_AUTO_NEG_STATUS,
		                              IXGBE_MDIO_AUTO_NEG_DEV_TYPE,
		                              &autoneg_reg);

		autoneg_reg &= IXGBE_MII_AUTONEG_COMPLETE;
		if (autoneg_reg == IXGBE_MII_AUTONEG_COMPLETE)
			break;
	}

	if (time_out == max_time_out) {
		status = IXGBE_ERR_LINK_SETUP;
		DEBUGOUT("ixgbe_setup_phy_link_tnx: time out");
	}

	return status;
}


/**
 *  ixgbe_get_phy_firmware_version_tnx - Gets the PHY Firmware Version
 *  @hw: pointer to hardware structure
 *  @firmware_version: pointer to the PHY Firmware Version
 **/
int32_t ixgbe_get_phy_firmware_version_tnx(struct ixgbe_hw *hw,
                                       uint16_t *firmware_version)
{
	int32_t status = IXGBE_SUCCESS;

	status = hw->phy.ops.read_reg(hw, TNX_FW_REV,
	                              IXGBE_MDIO_VENDOR_SPECIFIC_1_DEV_TYPE,
	                              firmware_version);

	return status;
}


/**
 *  ixgbe_get_phy_firmware_version_generic - Gets the PHY Firmware Version
 *  @hw: pointer to hardware structure
 *  @firmware_version: pointer to the PHY Firmware Version
 **/
int32_t ixgbe_get_phy_firmware_version_generic(struct ixgbe_hw *hw,
                                       uint16_t *firmware_version)
{
	int32_t status = IXGBE_SUCCESS;

	status = hw->phy.ops.read_reg(hw, AQ_FW_REV,
	                              IXGBE_MDIO_VENDOR_SPECIFIC_1_DEV_TYPE,
	                              firmware_version);

	return status;
}

/**
 *  ixgbe_reset_phy_nl - Performs a PHY reset
 *  @hw: pointer to hardware structure
 **/
int32_t ixgbe_reset_phy_nl(struct ixgbe_hw *hw)
{
	uint16_t phy_offset, control, eword, edata, block_crc; 
	int end_data = FALSE;
	uint16_t list_offset, data_offset;
	uint16_t phy_data = 0;
	int32_t ret_val = IXGBE_SUCCESS;
	uint32_t i;

	hw->phy.ops.read_reg(hw, IXGBE_MDIO_PHY_XS_CONTROL,
	                     IXGBE_MDIO_PHY_XS_DEV_TYPE, &phy_data);

	/* reset the PHY and poll for completion */
	hw->phy.ops.write_reg(hw, IXGBE_MDIO_PHY_XS_CONTROL,
	                      IXGBE_MDIO_PHY_XS_DEV_TYPE,
	                      (phy_data | IXGBE_MDIO_PHY_XS_RESET));

	for (i = 0; i < 100; i++) {
		hw->phy.ops.read_reg(hw, IXGBE_MDIO_PHY_XS_CONTROL,
		                     IXGBE_MDIO_PHY_XS_DEV_TYPE, &phy_data);
		if ((phy_data & IXGBE_MDIO_PHY_XS_RESET) == 0)
			break;
		msec_delay(10);
	}

	if ((phy_data & IXGBE_MDIO_PHY_XS_RESET) != 0) {
		DEBUGOUT("PHY reset did not complete.\n");
		ret_val = IXGBE_ERR_PHY;
		goto out;
	}

	/* Get init offsets */
	ret_val = ixgbe_get_sfp_init_sequence_offsets(hw, &list_offset,
	                                              &data_offset);
	if (ret_val != IXGBE_SUCCESS)
		goto out;

	ret_val = hw->eeprom.ops.read(hw, data_offset, &block_crc);
	data_offset++;
	while (!end_data) {
		/*
		 * Read control word from PHY init contents offset
		 */
		ret_val = hw->eeprom.ops.read(hw, data_offset, &eword);
		control = (eword & IXGBE_CONTROL_MASK_NL) >>
		           IXGBE_CONTROL_SHIFT_NL;
		edata = eword & IXGBE_DATA_MASK_NL;
		switch (control) {
		case IXGBE_DELAY_NL:
			data_offset++;
			DEBUGOUT1("DELAY: %d MS\n", edata);
			msec_delay(edata);
			break;
		case IXGBE_DATA_NL:
			DEBUGOUT("DATA:  \n");
			data_offset++;
			hw->eeprom.ops.read(hw, data_offset++,
			                    &phy_offset);
			for (i = 0; i < edata; i++) {
				hw->eeprom.ops.read(hw, data_offset, &eword);
				hw->phy.ops.write_reg(hw, phy_offset,
				                      IXGBE_TWINAX_DEV, eword);
				DEBUGOUT2("Wrote %4.4x to %4.4x\n", eword,
				          phy_offset);
				data_offset++;
				phy_offset++;
			}
			break;
		case IXGBE_CONTROL_NL:
			data_offset++;
			DEBUGOUT("CONTROL: \n");
			if (edata == IXGBE_CONTROL_EOL_NL) {
				DEBUGOUT("EOL\n");
				end_data = TRUE;
			} else if (edata == IXGBE_CONTROL_SOL_NL) {
				DEBUGOUT("SOL\n");
			} else {
				DEBUGOUT("Bad control value\n");
				ret_val = IXGBE_ERR_PHY;
				goto out;
			}
			break;
		default:
			DEBUGOUT("Bad control type\n");
			ret_val = IXGBE_ERR_PHY;
			goto out;
		}
	}

out:
	return ret_val;
}

/**
 *  ixgbe_identify_sfp_module_generic - Identifies SFP modules
 *  @hw: pointer to hardware structure
 *
 *  Searches for and identifies the SFP module and assigns appropriate PHY type.
 **/
int32_t ixgbe_identify_sfp_module_generic(struct ixgbe_hw *hw)
{
	int32_t status = IXGBE_ERR_PHY_ADDR_INVALID;
	uint32_t vendor_oui = 0;
	enum ixgbe_sfp_type stored_sfp_type = hw->phy.sfp_type;
	uint8_t identifier = 0;
	uint8_t comp_codes_1g = 0;
	uint8_t comp_codes_10g = 0;
	uint8_t oui_bytes[3] = {0, 0, 0};
	uint8_t cable_tech = 0;
	uint16_t enforce_sfp = 0;

	if (hw->mac.ops.get_media_type(hw) != ixgbe_media_type_fiber) {
		hw->phy.sfp_type = ixgbe_sfp_type_not_present;
		status = IXGBE_ERR_SFP_NOT_PRESENT;
		DEBUGOUT("SFP+ not present");
		goto out;
	}

	status = hw->phy.ops.read_i2c_eeprom(hw, IXGBE_SFF_IDENTIFIER,
	                                     &identifier);

	if (status == IXGBE_ERR_SFP_NOT_PRESENT || status == IXGBE_ERR_I2C) {
		status = IXGBE_ERR_SFP_NOT_PRESENT;
		hw->phy.sfp_type = ixgbe_sfp_type_not_present;
		if (hw->phy.type != ixgbe_phy_nl) {
			hw->phy.id = 0;
			hw->phy.type = ixgbe_phy_unknown;
		}
		goto out;
	}

	/* LAN ID is needed for sfp_type determination */
	hw->mac.ops.set_lan_id(hw);

	if (identifier != IXGBE_SFF_IDENTIFIER_SFP) {
		hw->phy.type = ixgbe_phy_sfp_unsupported;
		status = IXGBE_ERR_SFP_NOT_SUPPORTED;
	} else {
		hw->phy.ops.read_i2c_eeprom(hw, IXGBE_SFF_1GBE_COMP_CODES,
		                           &comp_codes_1g);
		hw->phy.ops.read_i2c_eeprom(hw, IXGBE_SFF_10GBE_COMP_CODES,
		                           &comp_codes_10g);
		hw->phy.ops.read_i2c_eeprom(hw, IXGBE_SFF_CABLE_TECHNOLOGY,
		                            &cable_tech);

		DEBUGOUT3("SFP+ capa codes 1G %x 10G %x cable %x\n",
		          comp_codes_1g, comp_codes_10g, cable_tech);

		 /* ID Module
		  * =========
		  * 0   SFP_DA_CU
		  * 1   SFP_SR
		  * 2   SFP_LR
		  * 3   SFP_DA_CORE0 - 82599-specific
		  * 4   SFP_DA_CORE1 - 82599-specific
		  * 5   SFP_SR/LR_CORE0 - 82599-specific
		  * 6   SFP_SR/LR_CORE1 - 82599-specific
		  */
		if (hw->mac.type == ixgbe_mac_82598EB) {
			if (cable_tech & IXGBE_SFF_DA_PASSIVE_CABLE)
				hw->phy.sfp_type = ixgbe_sfp_type_da_cu;
			else if (comp_codes_10g & IXGBE_SFF_10GBASESR_CAPABLE)
				hw->phy.sfp_type = ixgbe_sfp_type_sr;
			else if (comp_codes_10g & IXGBE_SFF_10GBASELR_CAPABLE)
				hw->phy.sfp_type = ixgbe_sfp_type_lr;
			else if (comp_codes_10g & IXGBE_SFF_DA_BAD_HP_CABLE)
				hw->phy.sfp_type = ixgbe_sfp_type_da_cu;
			else
				hw->phy.sfp_type = ixgbe_sfp_type_unknown;
		} else if (hw->mac.type == ixgbe_mac_82599EB) {
			if (cable_tech & IXGBE_SFF_DA_PASSIVE_CABLE)
				if (hw->bus.lan_id == 0)
					hw->phy.sfp_type =
					             ixgbe_sfp_type_da_cu_core0;
				else
					hw->phy.sfp_type =
					             ixgbe_sfp_type_da_cu_core1;
			else if (comp_codes_10g & IXGBE_SFF_10GBASESR_CAPABLE)
				if (hw->bus.lan_id == 0)
					hw->phy.sfp_type =
					              ixgbe_sfp_type_srlr_core0;
				else
					hw->phy.sfp_type =
					              ixgbe_sfp_type_srlr_core1;
			else if (comp_codes_10g & IXGBE_SFF_10GBASELR_CAPABLE)
				if (hw->bus.lan_id == 0)
					hw->phy.sfp_type =
					              ixgbe_sfp_type_srlr_core0;
				else
					hw->phy.sfp_type =
					              ixgbe_sfp_type_srlr_core1;
			else
				hw->phy.sfp_type = ixgbe_sfp_type_unknown;
		}

		if (hw->phy.sfp_type != stored_sfp_type)
			hw->phy.sfp_setup_needed = TRUE;

		/* Determine if the SFP+ PHY is dual speed or not. */
		hw->phy.multispeed_fiber = FALSE;
		if (((comp_codes_1g & IXGBE_SFF_1GBASESX_CAPABLE) &&
		   (comp_codes_10g & IXGBE_SFF_10GBASESR_CAPABLE)) ||
		   ((comp_codes_1g & IXGBE_SFF_1GBASELX_CAPABLE) &&
		   (comp_codes_10g & IXGBE_SFF_10GBASELR_CAPABLE)))
			hw->phy.multispeed_fiber = TRUE;

		/* Determine PHY vendor */
		if (hw->phy.type != ixgbe_phy_nl) {
			hw->phy.id = identifier;
			hw->phy.ops.read_i2c_eeprom(hw,
			                            IXGBE_SFF_VENDOR_OUI_BYTE0,
			                            &oui_bytes[0]);
			hw->phy.ops.read_i2c_eeprom(hw,
			                            IXGBE_SFF_VENDOR_OUI_BYTE1,
			                            &oui_bytes[1]);
			hw->phy.ops.read_i2c_eeprom(hw,
			                            IXGBE_SFF_VENDOR_OUI_BYTE2,
			                            &oui_bytes[2]);

			vendor_oui =
			   ((oui_bytes[0] << IXGBE_SFF_VENDOR_OUI_BYTE0_SHIFT) |
			    (oui_bytes[1] << IXGBE_SFF_VENDOR_OUI_BYTE1_SHIFT) |
			    (oui_bytes[2] << IXGBE_SFF_VENDOR_OUI_BYTE2_SHIFT));

			switch (vendor_oui) {
			case IXGBE_SFF_VENDOR_OUI_TYCO:
				if (cable_tech & IXGBE_SFF_DA_PASSIVE_CABLE)
					hw->phy.type = ixgbe_phy_tw_tyco;
				break;
			case IXGBE_SFF_VENDOR_OUI_FTL:
				hw->phy.type = ixgbe_phy_sfp_ftl;
				break;
			case IXGBE_SFF_VENDOR_OUI_AVAGO:
				hw->phy.type = ixgbe_phy_sfp_avago;
				break;
			case IXGBE_SFF_VENDOR_OUI_INTEL:
				hw->phy.type = ixgbe_phy_sfp_intel;
				break;
			default:
				if (cable_tech & IXGBE_SFF_DA_PASSIVE_CABLE)
					hw->phy.type = ixgbe_phy_tw_unknown;
				else
					hw->phy.type = ixgbe_phy_sfp_unknown;
				break;
			}
		}

		/* All passive DA cables are supported */
		if (cable_tech & IXGBE_SFF_DA_PASSIVE_CABLE) {
			status = IXGBE_SUCCESS;
			goto out;
		}

		/* 1G SFP modules are not supported */
		if (comp_codes_10g == 0) {
			hw->phy.type = ixgbe_phy_sfp_unsupported;
			status = IXGBE_ERR_SFP_NOT_SUPPORTED;
			goto out;
		}

		/* Anything else 82598-based is supported */
		if (hw->mac.type == ixgbe_mac_82598EB) {
			status = IXGBE_SUCCESS;
			goto out;
		}

		/* unimplemented even in the intel driver */
		/* ixgbe_get_device_caps(hw, &enforce_sfp); */
		enforce_sfp = IXGBE_DEVICE_CAPS_ALLOW_ANY_SFP;
		if (!(enforce_sfp & IXGBE_DEVICE_CAPS_ALLOW_ANY_SFP)) {
			/* Make sure we're a supported PHY type */
			if (hw->phy.type == ixgbe_phy_sfp_intel) {
				status = IXGBE_SUCCESS;
			} else {
				DEBUGOUT("SFP+ module not supported\n");
				hw->phy.type = ixgbe_phy_sfp_unsupported;
				status = IXGBE_ERR_SFP_NOT_SUPPORTED;
			}
		} else {
			DEBUGOUT("SFP+ module type ignored\n");
			status = IXGBE_SUCCESS;
		}
	}

out:
	return status;
}

/**
 *  ixgbe_get_sfp_init_sequence_offsets - Provides offset of PHY init sequence
 *  @hw: pointer to hardware structure
 *  @list_offset: offset to the SFP ID list
 *  @data_offset: offset to the SFP data block
 *
 *  Checks the MAC's EEPROM to see if it supports a given SFP+ module type, if
 *  so it returns the offsets to the phy init sequence block.
 **/
int32_t ixgbe_get_sfp_init_sequence_offsets(struct ixgbe_hw *hw,
                                        uint16_t *list_offset,
                                        uint16_t *data_offset)
{
	uint16_t sfp_id;

	if (hw->phy.sfp_type == ixgbe_sfp_type_unknown)
		return IXGBE_ERR_SFP_NOT_SUPPORTED;

	if (hw->phy.sfp_type == ixgbe_sfp_type_not_present)
		return IXGBE_ERR_SFP_NOT_PRESENT;

	if ((hw->device_id == IXGBE_DEV_ID_82598_SR_DUAL_PORT_EM) &&
	    (hw->phy.sfp_type == ixgbe_sfp_type_da_cu))
		return IXGBE_ERR_SFP_NOT_SUPPORTED;

	/* Read offset to PHY init contents */
	hw->eeprom.ops.read(hw, IXGBE_PHY_INIT_OFFSET_NL, list_offset);

	if ((!*list_offset) || (*list_offset == 0xFFFF))
		return IXGBE_ERR_SFP_NO_INIT_SEQ_PRESENT;

	/* Shift offset to first ID word */
	(*list_offset)++;

	/*
	 * Find the matching SFP ID in the EEPROM
	 * and program the init sequence
	 */
	hw->eeprom.ops.read(hw, *list_offset, &sfp_id);

	while (sfp_id != IXGBE_PHY_INIT_END_NL) {
		if (sfp_id == hw->phy.sfp_type) {
			(*list_offset)++;
			hw->eeprom.ops.read(hw, *list_offset, data_offset);
			if ((!*data_offset) || (*data_offset == 0xFFFF)) {
				DEBUGOUT("SFP+ module not supported\n");
				return IXGBE_ERR_SFP_NOT_SUPPORTED;
			} else {
				break;
			}
		} else {
			(*list_offset) += 2;
			if (hw->eeprom.ops.read(hw, *list_offset, &sfp_id))
				return IXGBE_ERR_PHY;
		}
	}

	/*
	 * the 82598EB SFP+ card offically supports only direct attached cables
	 * but works fine with optical SFP+ modules as well. Even though the
	 * EEPROM has no matching ID for them. So just accept the module.
	 */
	if (sfp_id == IXGBE_PHY_INIT_END_NL &&
	    hw->mac.type == ixgbe_mac_82598EB) {
		/* refetch offset for the first phy entry */
		hw->eeprom.ops.read(hw, IXGBE_PHY_INIT_OFFSET_NL, list_offset);
		(*list_offset) += 2;
		hw->eeprom.ops.read(hw, *list_offset, data_offset);
	} else if (sfp_id == IXGBE_PHY_INIT_END_NL)
		return IXGBE_ERR_SFP_NOT_SUPPORTED;

	return IXGBE_SUCCESS;
}

/**
 *  ixgbe_read_i2c_eeprom_generic - Reads 8 bit EEPROM word over I2C interface
 *  @hw: pointer to hardware structure
 *  @byte_offset: EEPROM byte offset to read
 *  @eeprom_data: value read
 *
 *  Performs byte read operation to SFP module's EEPROM over I2C interface.
 **/
int32_t ixgbe_read_i2c_eeprom_generic(struct ixgbe_hw *hw, uint8_t byte_offset,
                                  uint8_t *eeprom_data)
{
	return hw->phy.ops.read_i2c_byte(hw, byte_offset,
	                                 IXGBE_I2C_EEPROM_DEV_ADDR,
	                                 eeprom_data);
}

/**
 *  ixgbe_write_i2c_eeprom_generic - Writes 8 bit EEPROM word over I2C interface
 *  @hw: pointer to hardware structure
 *  @byte_offset: EEPROM byte offset to write
 *  @eeprom_data: value to write
 *
 *  Performs byte write operation to SFP module's EEPROM over I2C interface.
 **/
int32_t ixgbe_write_i2c_eeprom_generic(struct ixgbe_hw *hw, uint8_t byte_offset,
                                   uint8_t eeprom_data)
{
	return hw->phy.ops.write_i2c_byte(hw, byte_offset,
	                                  IXGBE_I2C_EEPROM_DEV_ADDR,
	                                  eeprom_data);
}

/**
 *  ixgbe_read_i2c_byte_generic - Reads 8 bit word over I2C
 *  @hw: pointer to hardware structure
 *  @byte_offset: byte offset to read
 *  @data: value read
 *
 *  Performs byte read operation to SFP module's EEPROM over I2C interface at
 *  a specified deivce address.
 **/
int32_t ixgbe_read_i2c_byte_generic(struct ixgbe_hw *hw, uint8_t byte_offset,
                                uint8_t dev_addr, uint8_t *data)
{
	int32_t status = IXGBE_SUCCESS;
	uint32_t max_retry = 10;
	uint32_t retry = 0;
	uint16_t swfw_mask = 0;
	int nack = 1;

	if (IXGBE_READ_REG(hw, IXGBE_STATUS) & IXGBE_STATUS_LAN_ID_1)
		swfw_mask = IXGBE_GSSR_PHY1_SM;
	else
		swfw_mask = IXGBE_GSSR_PHY0_SM;


	do {
		if (ixgbe_acquire_swfw_sync(hw, swfw_mask) != IXGBE_SUCCESS) {
			status = IXGBE_ERR_SWFW_SYNC;
			goto read_byte_out;
		}

		ixgbe_i2c_start(hw);

		/* Device Address and write indication */
		status = ixgbe_clock_out_i2c_byte(hw, dev_addr);
		if (status != IXGBE_SUCCESS)
			goto fail;

		status = ixgbe_get_i2c_ack(hw);
		if (status != IXGBE_SUCCESS)
			goto fail;

		status = ixgbe_clock_out_i2c_byte(hw, byte_offset);
		if (status != IXGBE_SUCCESS)
			goto fail;

		status = ixgbe_get_i2c_ack(hw);
		if (status != IXGBE_SUCCESS)
			goto fail;

		ixgbe_i2c_start(hw);

		/* Device Address and read indication */
		status = ixgbe_clock_out_i2c_byte(hw, (dev_addr | 0x1));
		if (status != IXGBE_SUCCESS)
			goto fail;

		status = ixgbe_get_i2c_ack(hw);
		if (status != IXGBE_SUCCESS)
			goto fail;

		status = ixgbe_clock_in_i2c_byte(hw, data);
		if (status != IXGBE_SUCCESS)
			goto fail;

		status = ixgbe_clock_out_i2c_bit(hw, nack);
		if (status != IXGBE_SUCCESS)
			goto fail;

		ixgbe_i2c_stop(hw);
		break;

fail:
		ixgbe_release_swfw_sync(hw, swfw_mask);
		msec_delay(100);
		ixgbe_i2c_bus_clear(hw);
		retry++;
		if (retry < max_retry)
			DEBUGOUT("I2C byte read error - Retrying.\n");
		else
			DEBUGOUT("I2C byte read error.\n");

	} while (retry < max_retry);

	ixgbe_release_swfw_sync(hw, swfw_mask);

read_byte_out:
	return status;
}

/**
 *  ixgbe_write_i2c_byte_generic - Writes 8 bit word over I2C
 *  @hw: pointer to hardware structure
 *  @byte_offset: byte offset to write
 *  @data: value to write
 *
 *  Performs byte write operation to SFP module's EEPROM over I2C interface at
 *  a specified device address.
 **/
int32_t ixgbe_write_i2c_byte_generic(struct ixgbe_hw *hw, uint8_t byte_offset,
                                 uint8_t dev_addr, uint8_t data)
{
	int32_t status = IXGBE_SUCCESS;
	uint32_t max_retry = 1;
	uint32_t retry = 0;
	uint16_t swfw_mask = 0;

	if (IXGBE_READ_REG(hw, IXGBE_STATUS) & IXGBE_STATUS_LAN_ID_1)
		swfw_mask = IXGBE_GSSR_PHY1_SM;
	else
		swfw_mask = IXGBE_GSSR_PHY0_SM;

	if (ixgbe_acquire_swfw_sync(hw, swfw_mask) != IXGBE_SUCCESS) {
		status = IXGBE_ERR_SWFW_SYNC;
		goto write_byte_out;
	}

	do {
		ixgbe_i2c_start(hw);

		status = ixgbe_clock_out_i2c_byte(hw, dev_addr);
		if (status != IXGBE_SUCCESS)
			goto fail;

		status = ixgbe_get_i2c_ack(hw);
		if (status != IXGBE_SUCCESS)
			goto fail;

		status = ixgbe_clock_out_i2c_byte(hw, byte_offset);
		if (status != IXGBE_SUCCESS)
			goto fail;

		status = ixgbe_get_i2c_ack(hw);
		if (status != IXGBE_SUCCESS)
			goto fail;

		status = ixgbe_clock_out_i2c_byte(hw, data);
		if (status != IXGBE_SUCCESS)
			goto fail;

		status = ixgbe_get_i2c_ack(hw);
		if (status != IXGBE_SUCCESS)
			goto fail;

		ixgbe_i2c_stop(hw);
		break;

fail:
		ixgbe_i2c_bus_clear(hw);
		retry++;
		if (retry < max_retry)
			DEBUGOUT("I2C byte write error - Retrying.\n");
		else
			DEBUGOUT("I2C byte write error.\n");
	} while (retry < max_retry);

	ixgbe_release_swfw_sync(hw, swfw_mask);

write_byte_out:
	return status;
}

/**
 *  ixgbe_i2c_start - Sets I2C start condition
 *  @hw: pointer to hardware structure
 *
 *  Sets I2C start condition (High -> Low on SDA while SCL is High)
 **/
void ixgbe_i2c_start(struct ixgbe_hw *hw)
{
	uint32_t i2cctl = IXGBE_READ_REG(hw, IXGBE_I2CCTL);

	/* Start condition must begin with data and clock high */
	ixgbe_set_i2c_data(hw, &i2cctl, 1);
	ixgbe_raise_i2c_clk(hw, &i2cctl);

	/* Setup time for start condition (4.7us) */
	usec_delay(IXGBE_I2C_T_SU_STA);

	ixgbe_set_i2c_data(hw, &i2cctl, 0);

	/* Hold time for start condition (4us) */
	usec_delay(IXGBE_I2C_T_HD_STA);

	ixgbe_lower_i2c_clk(hw, &i2cctl);

	/* Minimum low period of clock is 4.7 us */
	usec_delay(IXGBE_I2C_T_LOW);

}

/**
 *  ixgbe_i2c_stop - Sets I2C stop condition
 *  @hw: pointer to hardware structure
 *
 *  Sets I2C stop condition (Low -> High on SDA while SCL is High)
 **/
void ixgbe_i2c_stop(struct ixgbe_hw *hw)
{
	uint32_t i2cctl = IXGBE_READ_REG(hw, IXGBE_I2CCTL);

	/* Stop condition must begin with data low and clock high */
	ixgbe_set_i2c_data(hw, &i2cctl, 0);
	ixgbe_raise_i2c_clk(hw, &i2cctl);

	/* Setup time for stop condition (4us) */
	usec_delay(IXGBE_I2C_T_SU_STO);

	ixgbe_set_i2c_data(hw, &i2cctl, 1);

	/* bus free time between stop and start (4.7us)*/
	usec_delay(IXGBE_I2C_T_BUF);
}

/**
 *  ixgbe_clock_in_i2c_byte - Clocks in one byte via I2C
 *  @hw: pointer to hardware structure
 *  @data: data byte to clock in
 *
 *  Clocks in one byte data via I2C data/clock
 **/
int32_t ixgbe_clock_in_i2c_byte(struct ixgbe_hw *hw, uint8_t *data)
{
	int32_t status = IXGBE_SUCCESS;
	int32_t i;
	int bit = 0;

	for (i = 7; i >= 0; i--) {
		status = ixgbe_clock_in_i2c_bit(hw, &bit);
		*data |= bit<<i;

		if (status != IXGBE_SUCCESS)
			break;
	}

	return status;
}

/**
 *  ixgbe_clock_out_i2c_byte - Clocks out one byte via I2C
 *  @hw: pointer to hardware structure
 *  @data: data byte clocked out
 *
 *  Clocks out one byte data via I2C data/clock
 **/
int32_t ixgbe_clock_out_i2c_byte(struct ixgbe_hw *hw, uint8_t data)
{
	int32_t status = IXGBE_SUCCESS;
	int32_t i;
	uint32_t i2cctl;
	int bit = 0;

	for (i = 7; i >= 0; i--) {
		bit = (data >> i) & 0x1;
		status = ixgbe_clock_out_i2c_bit(hw, bit);

		if (status != IXGBE_SUCCESS)
			break;
	}

	/* Release SDA line (set high) */
	i2cctl = IXGBE_READ_REG(hw, IXGBE_I2CCTL);
	i2cctl |= IXGBE_I2C_DATA_OUT;
	IXGBE_WRITE_REG(hw, IXGBE_I2CCTL, i2cctl);

	return status;
}

/**
 *  ixgbe_get_i2c_ack - Polls for I2C ACK
 *  @hw: pointer to hardware structure
 *
 *  Clocks in/out one bit via I2C data/clock
 **/
int32_t ixgbe_get_i2c_ack(struct ixgbe_hw *hw)
{
	int32_t status;
	uint32_t i = 0;
	uint32_t i2cctl = IXGBE_READ_REG(hw, IXGBE_I2CCTL);
	uint32_t timeout = 10;
	int ack = 1;

	status = ixgbe_raise_i2c_clk(hw, &i2cctl);

	if (status != IXGBE_SUCCESS)
		goto out;

	/* Minimum high period of clock is 4us */
	usec_delay(IXGBE_I2C_T_HIGH);

	/* Poll for ACK.  Note that ACK in I2C spec is
	 * transition from 1 to 0 */
	for (i = 0; i < timeout; i++) {
		i2cctl = IXGBE_READ_REG(hw, IXGBE_I2CCTL);
		ack = ixgbe_get_i2c_data(&i2cctl);

		usec_delay(1);
		if (ack == 0)
			break;
	}

	if (ack == 1) {
		DEBUGOUT("I2C ack was not received.\n");
		status = IXGBE_ERR_I2C;
	}

	ixgbe_lower_i2c_clk(hw, &i2cctl);

	/* Minimum low period of clock is 4.7 us */
	usec_delay(IXGBE_I2C_T_LOW);

out:
	return status;
}

/**
 *  ixgbe_clock_in_i2c_bit - Clocks in one bit via I2C data/clock
 *  @hw: pointer to hardware structure
 *  @data: read data value
 *
 *  Clocks in one bit via I2C data/clock
 **/
int32_t ixgbe_clock_in_i2c_bit(struct ixgbe_hw *hw, int *data)
{
	int32_t status;
	uint32_t i2cctl = IXGBE_READ_REG(hw, IXGBE_I2CCTL);

	status = ixgbe_raise_i2c_clk(hw, &i2cctl);

	/* Minimum high period of clock is 4us */
	usec_delay(IXGBE_I2C_T_HIGH);

	i2cctl = IXGBE_READ_REG(hw, IXGBE_I2CCTL);
	*data = ixgbe_get_i2c_data(&i2cctl);

	ixgbe_lower_i2c_clk(hw, &i2cctl);

	/* Minimum low period of clock is 4.7 us */
	usec_delay(IXGBE_I2C_T_LOW);

	return status;
}

/**
 *  ixgbe_clock_out_i2c_bit - Clocks in/out one bit via I2C data/clock
 *  @hw: pointer to hardware structure
 *  @data: data value to write
 *
 *  Clocks out one bit via I2C data/clock
 **/
int32_t ixgbe_clock_out_i2c_bit(struct ixgbe_hw *hw, int data)
{
	int32_t status;
	uint32_t i2cctl = IXGBE_READ_REG(hw, IXGBE_I2CCTL);

	status = ixgbe_set_i2c_data(hw, &i2cctl, data);
	if (status == IXGBE_SUCCESS) {
		status = ixgbe_raise_i2c_clk(hw, &i2cctl);

		/* Minimum high period of clock is 4us */
		usec_delay(IXGBE_I2C_T_HIGH);

		ixgbe_lower_i2c_clk(hw, &i2cctl);

		/* Minimum low period of clock is 4.7 us.
		 * This also takes care of the data hold time.
		 */
		usec_delay(IXGBE_I2C_T_LOW);
	} else {
		status = IXGBE_ERR_I2C;
		DEBUGOUT1("I2C data was not set to %X\n", data);
	}

	return status;
}
/**
 *  ixgbe_raise_i2c_clk - Raises the I2C SCL clock
 *  @hw: pointer to hardware structure
 *  @i2cctl: Current value of I2CCTL register
 *
 *  Raises the I2C clock line '0'->'1'
 **/
int32_t ixgbe_raise_i2c_clk(struct ixgbe_hw *hw, uint32_t *i2cctl)
{
	int32_t status = IXGBE_SUCCESS;

	*i2cctl |= IXGBE_I2C_CLK_OUT;

	IXGBE_WRITE_REG(hw, IXGBE_I2CCTL, *i2cctl);

	/* SCL rise time (1000ns) */
	usec_delay(IXGBE_I2C_T_RISE);

	return status;
}

/**
 *  ixgbe_lower_i2c_clk - Lowers the I2C SCL clock
 *  @hw: pointer to hardware structure
 *  @i2cctl: Current value of I2CCTL register
 *
 *  Lowers the I2C clock line '1'->'0'
 **/
void ixgbe_lower_i2c_clk(struct ixgbe_hw *hw, uint32_t *i2cctl)
{

	*i2cctl &= ~IXGBE_I2C_CLK_OUT;

	IXGBE_WRITE_REG(hw, IXGBE_I2CCTL, *i2cctl);

	/* SCL fall time (300ns) */
	usec_delay(IXGBE_I2C_T_FALL);
}

/**
 *  ixgbe_set_i2c_data - Sets the I2C data bit
 *  @hw: pointer to hardware structure
 *  @i2cctl: Current value of I2CCTL register
 *  @data: I2C data value (0 or 1) to set
 *
 *  Sets the I2C data bit
 **/
int32_t ixgbe_set_i2c_data(struct ixgbe_hw *hw, uint32_t *i2cctl, int data)
{
	int32_t status = IXGBE_SUCCESS;

	if (data)
		*i2cctl |= IXGBE_I2C_DATA_OUT;
	else
		*i2cctl &= ~IXGBE_I2C_DATA_OUT;

	IXGBE_WRITE_REG(hw, IXGBE_I2CCTL, *i2cctl);

	/* Data rise/fall (1000ns/300ns) and set-up time (250ns) */
	usec_delay(IXGBE_I2C_T_RISE + IXGBE_I2C_T_FALL + IXGBE_I2C_T_SU_DATA);

	/* Verify data was set correctly */
	*i2cctl = IXGBE_READ_REG(hw, IXGBE_I2CCTL);
	if (data != ixgbe_get_i2c_data(i2cctl)) {
		status = IXGBE_ERR_I2C;
		DEBUGOUT1("Error - I2C data was not set to %X.\n", data);
	}

	return status;
}

/**
 *  ixgbe_get_i2c_data - Reads the I2C SDA data bit
 *  @hw: pointer to hardware structure
 *  @i2cctl: Current value of I2CCTL register
 *
 *  Returns the I2C data bit value
 **/
int ixgbe_get_i2c_data(uint32_t *i2cctl)
{
	int data;

	if (*i2cctl & IXGBE_I2C_DATA_IN)
		data = 1;
	else
		data = 0;

	return data;
}

/**
 *  ixgbe_i2c_bus_clear - Clears the I2C bus
 *  @hw: pointer to hardware structure
 *
 *  Clears the I2C bus by sending nine clock pulses.
 *  Used when data line is stuck low.
 **/
void ixgbe_i2c_bus_clear(struct ixgbe_hw *hw)
{
	uint32_t i2cctl = IXGBE_READ_REG(hw, IXGBE_I2CCTL);
	uint32_t i;

	ixgbe_i2c_start(hw);

	ixgbe_set_i2c_data(hw, &i2cctl, 1);

	for (i = 0; i < 9; i++) {
		ixgbe_raise_i2c_clk(hw, &i2cctl);

		/* Min high period of clock is 4us */
		usec_delay(IXGBE_I2C_T_HIGH);

		ixgbe_lower_i2c_clk(hw, &i2cctl);

		/* Min low period of clock is 4.7us*/
		usec_delay(IXGBE_I2C_T_LOW);
	}

	ixgbe_i2c_start(hw);

	/* Put the i2c bus back to default state */
	ixgbe_i2c_stop(hw);
}
