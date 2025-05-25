DROP TABLE [TCastleRewardUsers]
go
DROP TABLE [TCastleSaveList]
go


DROP PROCEDURE [gp_Castle_SaveBattleData]
go
DROP PROCEDURE [gp_Castle_LoadBattleData]
go


CREATE TABLE [dbo].[TCastleSaveList](
	[castleIndex] [int] NOT NULL,
	[SaveType] [int] NOT NULL,
	[SaveIndex] [int] NOT NULL,
	[Value1] [int] NOT NULL,
	[Value2] [int] NOT NULL,
	[LogTime] [datetime] NOT NULL

) ON [PRIMARY]
GO
ALTER TABLE [dbo].[TCastleSaveList] ADD  CONSTRAINT [DF_TCastleSaveList_Value1]  DEFAULT ((0)) FOR [Value1]
GO
ALTER TABLE [dbo].[TCastleSaveList] ADD  CONSTRAINT [DF_TCastleSaveList_Value2]  DEFAULT ((0)) FOR [Value2]
GO
ALTER TABLE [dbo].[TCastleSaveList] ADD  CONSTRAINT [DF_TCastleSaveList_LogTime]  DEFAULT (getdate()) FOR [LogTime]
GO

CREATE PROCEDURE [dbo].[gp_Castle_SaveBattleData]

/*1*/@castleIdx int,
/*2*/@saveType int,
/*3*/@saveIndex int, 
/*4*/@deleteType int, 
/*5*/@val1 int, 
/*6*/@val2 int


AS
BEGIN
	if(@deleteType = 0) --데이터 입력
	begin
	
		if exists (SELECT 1 from [TCastleSaveList] where @castleIdx = [castleIndex] and @saveType = [SaveType] and @saveIndex = [SaveIndex])
		begin
			UPDATE [TCastleSaveList] set [Value1] = @val1, [Value2] = @val2 where [castleIndex] = @castleIdx and @saveType = [SaveType] and @saveIndex = [SaveIndex]
		end
		else
		begin
			INSERT INTO [TCastleSaveList]([castleIndex],[SaveType] ,[SaveIndex],[Value1] ,[Value2])
			VALUES(@castleIdx,@saveType,@saveIndex, @val1,@val2)
		end
	end
	else if(@deleteType = 1) --삭제
	begin
		delete [TCastleSaveList] where castleIndex = @castleIdx
	end
END


GO


CREATE PROCEDURE [dbo].[gp_Castle_LoadBattleData]
/*1*/@castleIdx int

AS
BEGIN
 SELECT 
	[SaveType],
	[SaveIndex],
	[Value1] ,
	[Value2] 

 FROM [TCastleSaveList]
 WHERE [castleIndex] = @castleIdx
 order by [SaveType] asc

END