// INCLUDES ------------------------------------------------------------------------------------------------------------------------

#include "uart_machine.h"

// VARIABLES ------------------------------------------------------------------------------------------------------------------------

LOG_MODULE_REGISTER(LOG_UART, LOG_UART_LEVEL);

static uint8_t uart_machine_state       = UART_INIT;

static bool app_notifi_error            = false;
static bool app_notifi_ready            = false;

static uint8_t current_buffer_index     = 0;
static uint16_t uart_data_index         = 0;
static uint16_t app_data_index          = 0;
static uint16_t serch_data_index        = 0;

const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart1));

const struct uart_config uart_config = {.baudrate   = UART_BAUDRATE,
		                                .parity     = UART_PARITY,
		                                .stop_bits  = UART_STOP_BITS ,
		                                .data_bits  = UART_DATA_BITS ,
		                                .flow_ctrl  = UART_FLOW_CTRL };

uint8_t uart_rx_buffers[RECEIVE_BUFF_NUMBER][RECEIVE_BUFF_SIZE];

// FUNCTIONS GET ------------------------------------------------------------------------------------------------------------------------

bool get_UART_notifi_error(void)
{
    return app_notifi_error;
}

bool get_UART_notifi_ready(void)
{
    return app_notifi_ready;
}

uint8_t get_rdy_data()
{
    uint8_t data;
    if(app_data_index != uart_data_index)
    {
        if( ((app_data_index+LEN_CODE)%(RECEIVE_BUFF_NUMBER*RECEIVE_BUFF_SIZE)) <= serch_data_index)
        {
            data = * ( *((uint8_t (*)[RECEIVE_BUFF_NUMBER*RECEIVE_BUFF_SIZE])uart_rx_buffers) + app_data_index);
            app_data_index ++;
            app_data_index %= (RECEIVE_BUFF_NUMBER*RECEIVE_BUFF_SIZE);
            
            //LOG_DBG("UART get_rdy_data : uart index = %d - app index = %d - caracter = %c\n", uart_data_index, app_data_index, data);
            return data;
        }
    }
    return 0;
}

// FUNCTIONS ------------------------------------------------------------------------------------------------------------------------

/*void serch_code(Code_callback_t cb)
{
    uint8_t data;
    static uint8_t last_data;
    static uint8_t code[LEN_CODE]   = CODE;
    static uint8_t counter          = 0;
    static bool    flag_code        = false;
    static uint8_t len_code         = 0;
    static uint8_t code_index       = 0;
    
    while(serch_data_index != uart_data_index)
    {
        data = * ( *((uint8_t (*)[RECEIVE_BUFF_NUMBER*RECEIVE_BUFF_SIZE])uart_rx_buffers) + serch_data_index);
        serch_data_index ++;
        serch_data_index %= (RECEIVE_BUFF_NUMBER*RECEIVE_BUFF_SIZE);
        if(flag_code == true)
        {
            if(len_code == 0) len_code = data;
            else {
                counter ++;
                if(counter == len_code)
                {
                    cb(serch_data_index, ((uint8_t (*)[RECEIVE_BUFF_NUMBER*RECEIVE_BUFF_SIZE])uart_rx_buffers) ,data);
                    flag_code = false;
                    counter   = 0;
                    len_code  = 0;
                }
            }
            return;
        }
        if(data == code[counter]) counter++;
        else counter = 0;
        if(counter == LEN_CODE-1) {
            flag_code = true; 
            counter   = 0;
            len_code  = 0;
        }
    }
}*/

// CALLBACK ------------------------------------------------------------------------------------------------------------------------

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) 
	{
        case UART_RX_BUF_REQUEST:
        {
            LOG_DBG("UART_RX_BUF_REQUEST\n");
            uart_rx_buf_rsp(uart,
                            uart_rx_buffers[(current_buffer_index+1)%RECEIVE_BUFF_NUMBER],
                            RECEIVE_BUFF_SIZE);
            break;
        }

		case UART_RX_RDY:
		{
            LOG_DBG("UART_RX_RDY\n");
            uart_data_index = uart_data_index + evt->data.rx.len;
            uart_data_index %= (RECEIVE_BUFF_NUMBER*RECEIVE_BUFF_SIZE);
			break;
		}

        case UART_RX_BUF_RELEASED:
        {
            LOG_DBG("UART_RX_BUF_RELEASED\n");
            current_buffer_index ++;
            current_buffer_index %= RECEIVE_BUFF_NUMBER;
            break;
        }

		case UART_RX_DISABLED:
		{
            LOG_DBG("UART_RX_DISABLED\n");
            //uart_machine_state = WAIT_FOT_REINIT;   // FRAN usar esto aca traia problemas pero no me acuerdo por que
            app_notifi_error = true;
            break;
		}

        case UART_RX_STOPPED:
        {
            LOG_ERR("UART_RX_STOP: ");
            app_notifi_error = true;
            switch (evt->data.rx_stop.reason)
            {
                case UART_ERROR_OVERRUN:
                    LOG_ERR("OVERRUN\n");
                    break;
            
                case UART_ERROR_PARITY:
                    LOG_ERR("PARITY\n");
                    break;

                case UART_ERROR_FRAMING:
                    LOG_ERR("FRAMING\n");
                    break;

                case UART_BREAK:
                    LOG_ERR("BREAK\n");
                    break;

                case UART_ERROR_COLLISION:
                    LOG_ERR("COLLISION\n");
                    break;

                case UART_ERROR_NOISE:
                    LOG_ERR("NOISE\n");
                    break;
            }
            break;
        }

		default:
		{
			LOG_DBG("Default - Event: %d\n" , evt->type);
			break;
		}
	}
}

// FUNCTIONS MACHINE ------------------------------------------------------------------------------------------------------------------------

void uart_machine()
{
    uint8_t err;

    switch (uart_machine_state)
    {
        case UART_INIT:
        {
            if (!device_is_ready(uart)) {
		        LOG_ERR("UART device is not ready\n");
                app_notifi_error = true;
                uart_machine_state = UART_DEACTIVATE;
		        break;
	        }

	        err = uart_configure(uart, &uart_config);
	        if (err == -ENOSYS) {
		        LOG_ERR("Init UART failed\n");
                app_notifi_error = true;
                uart_machine_state = UART_DEACTIVATE;
		        break;
	        }

	        err = uart_callback_set(uart, uart_cb, NULL);
	        if (err) {
		        LOG_ERR("Set UART Callback failed\n");
                app_notifi_error = true;
                uart_machine_state = UART_DEACTIVATE;
		        break;
	        }

            uart_machine_state = UART_RX_INIT;
            break;
        }

        case UART_RX_INIT:
        {
            current_buffer_index = 0;
            uart_data_index      = 0;
            app_data_index       = 0;
            serch_data_index     = 0;

            if (uart_rx_enable(uart ,uart_rx_buffers[current_buffer_index] ,RECEIVE_BUFF_SIZE ,RECEIVE_TIMEOUT_HAR_US)) {
		        LOG_ERR("UART RX enabled failed\n");
                app_notifi_error = true;
                uart_machine_state = UART_DEACTIVATE;
                break;
	        }

            LOG_INF("UART STATE WORKING\n");
            uart_machine_state = UART_WORKING;
            app_notifi_ready = true;
            break;
        }
    
        case UART_WORKING:
        {
            
            break;
        }

        case UART_DEACTIVATE:     //FRAN
        {
            LOG_INF("UART_DEACTIVATE\n");
            uart_rx_disable(uart);
            uart_machine_state = UART_INIT;
            app_notifi_ready   = false;
            break;
        }

        case WAIT_FOT_REINIT:     //FRAN
        {
            LOG_INF("UART WAIT_FOT_REINIT\n");
            k_sleep(K_MSEC(K_INTERVAL_TO_REINIT));
            uart_machine_state = UART_INIT;
            app_notifi_ready   = false;
            break;
        }

        default:
        {
            LOG_ERR("UART machine state default\n");
            uart_machine_state = UART_DEACTIVATE;
            break;
        }
    }
}

// ------------------------------------------------------------------------------------------------------------------------