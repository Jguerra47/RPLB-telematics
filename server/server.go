package main 


import (
	"github.com/gin-gonic/gin"
)

func main() {
	router := gin.Default()

	router.GET("/ping", func(ctx *gin.Context) {
		ctx.JSON(200, gin.H{
			"message": "pong",
			"status":"200",
		})
	})

	router.GET("/",func(ctx *gin.Context){
		ctx.JSON(200,gin.H{"message":"Default"})
	})

	router.Run()
}
